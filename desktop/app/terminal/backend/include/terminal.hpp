#pragma once

#include <string>
#include <pty.h>
#include <utmp.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <csignal>
#include <cstring>

struct TermSession {
    int master_fd = -1;
    pid_t child_pid = -1;

    ~TermSession() { close(); }

    bool start(const std::string& cmd) {
        close();

        struct winsize ws;
        ws.ws_row = 24;
        ws.ws_col = 80;
        ws.ws_xpixel = 0;
        ws.ws_ypixel = 0;

        pid_t pid = forkpty(&master_fd, nullptr, nullptr, &ws);
        if (pid == 0) {
            // Child: set up and exec
            signal(SIGINT, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGTERM, SIG_DFL);
            if (!cmd.empty())
                execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
            else
                execl("/bin/sh", "sh", "--norc", nullptr);
            _exit(127);
        }
        if (pid < 0) return false;

        child_pid = pid;

        // Non-blocking read
        int flags = fcntl(master_fd, F_GETFL, 0);
        fcntl(master_fd, F_SETFL, flags | O_NONBLOCK);

        return true;
    }

    std::string readOutput() {
        std::string result;
        char buf[4096];
        usleep(30000);
        for (int i = 0; i < 5; i++) {
            bool got = false;
            while (true) {
                ssize_t n = read(master_fd, buf, sizeof(buf));
                if (n > 0) { result.append(buf, n); got = true; }
                else break;
            }
            if (!got) {
                if (!result.empty()) break;
                usleep(40000);
            } else {
                usleep(20000);
            }
        }
        return result;
    }

    std::string readPending() {
        std::string result;
        char buf[4096];
        while (true) {
            ssize_t n = read(master_fd, buf, sizeof(buf));
            if (n > 0) result.append(buf, n);
            else break;
        }
        return result;
    }

    void writeInput(const std::string& data) {
        if (master_fd >= 0 && !data.empty())
            write(master_fd, data.data(), data.size());
    }

    void setSize(int rows, int cols) {
        if (master_fd < 0) return;
        struct winsize ws;
        ioctl(master_fd, TIOCGWINSZ, &ws);
        ws.ws_row = rows;
        ws.ws_col = cols;
        ioctl(master_fd, TIOCSWINSZ, &ws);
    }

    bool isAlive() {
        if (child_pid <= 0) return false;
        int status;
        pid_t r = waitpid(child_pid, &status, WNOHANG);
        if (r == child_pid) {
            child_pid = -1;
            return false;
        }
        return r == 0;
    }

    void close() {
        if (child_pid > 0) {
            kill(child_pid, SIGTERM);
            usleep(50000);
            kill(child_pid, SIGKILL);
            waitpid(child_pid, nullptr, WNOHANG);
            child_pid = -1;
        }
        if (master_fd >= 0) {
            ::close(master_fd);
            master_fd = -1;
        }
    }
};
