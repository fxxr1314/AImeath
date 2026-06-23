import { createChannel } from './channel.js'

const WS_URL = `ws://${location.hostname}:3001`

export function createGameSocket(game, width = 20, height = 20) {
  const ch = createChannel(WS_URL, { maxRetries: 5 })
  const stateListeners = new Set()

  ch.onOpen(() => ch.send({ action: 'new_game', game, width, height }))
  ch.onMessage(data => stateListeners.forEach(fn => fn(data)))
  ch.onError(() => stateListeners.forEach(fn => fn({ type: 'error', msg: 'Connection error' })))
  ch.onReconnecting(({ attempt, max, delay }) =>
    stateListeners.forEach(fn => fn({ type: 'reconnecting', attempt, max, delay })))
  ch.onClose(() => stateListeners.forEach(fn => fn({ type: 'closed' })))

  return {
    send(payload) { ch.send(payload) },
    tick(value) { ch.send({ action: 'tick', value }) },
    endGame() { ch.send({ action: 'end_game' }); ch.close() },
    onState(fn) {
      stateListeners.add(fn)
      return () => stateListeners.delete(fn)
    },
    close() { ch.close() },
  }
}
