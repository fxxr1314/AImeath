const tree = [
  {
    name: 'Documents', type: 'dir', modified: '2026-06-20',
    children: [
      {
        name: 'work', type: 'dir', modified: '2026-06-18',
        children: [
          { name: 'project-plan.md', type: 'file', size: 2048, modified: '2026-06-15' },
          { name: 'meeting-notes.txt', type: 'file', size: 1024, modified: '2026-06-12' },
          { name: 'architecture.pdf', type: 'file', size: 320000, modified: '2026-06-10' },
          { name: 'CMakeLists.txt', type: 'file', size: 512, modified: '2026-06-08' },
        ],
      },
      {
        name: 'personal', type: 'dir', modified: '2026-06-01',
        children: [
          { name: 'diary.txt', type: 'file', size: 4096, modified: '2026-06-01' },
          { name: 'budget.csv', type: 'file', size: 2048, modified: '2026-05-28' },
        ],
      },
      { name: 'resume.pdf', type: 'file', size: 154000, modified: '2026-05-20' },
      { name: 'cover-letter.docx', type: 'file', size: 28000, modified: '2026-05-19' },
    ],
  },
  {
    name: 'Downloads', type: 'dir', modified: '2026-06-22',
    children: [
      { name: 'vscode_amd64.deb', type: 'file', size: 95000000, modified: '2026-06-22' },
      { name: 'wallpaper.jpg', type: 'file', size: 2400000, modified: '2026-06-20' },
      { name: 'node-v20.tar.gz', type: 'file', size: 42000000, modified: '2026-06-18' },
      { name: 'screenshot.png', type: 'file', size: 1200000, modified: '2026-06-15' },
    ],
  },
  {
    name: 'Pictures', type: 'dir', modified: '2026-06-19',
    children: [
      {
        name: 'vacation', type: 'dir', modified: '2026-05-10',
        children: [
          { name: 'sunset.jpg', type: 'file', size: 3200000, modified: '2026-05-10' },
          { name: 'group-photo.jpg', type: 'file', size: 4500000, modified: '2026-05-09' },
        ],
      },
      { name: 'desktop-bg.png', type: 'file', size: 1800000, modified: '2026-04-15' },
    ],
  },
  {
    name: 'Music', type: 'dir', modified: '2026-06-05',
    children: [
      { name: 'playlist.m3u', type: 'file', size: 512, modified: '2026-06-05' },
      { name: 'album-art.jpg', type: 'file', size: 800000, modified: '2026-06-01' },
    ],
  },
  {
    name: 'Videos', type: 'dir', modified: '2026-06-10',
    children: [
      { name: 'tutorial.mp4', type: 'file', size: 120000000, modified: '2026-06-10' },
    ],
  },
  {
    name: 'Projects', type: 'dir', modified: '2026-06-23',
    children: [
      {
        name: 'snake', type: 'dir', modified: '2026-06-20',
        children: [
          { name: 'main.cpp', type: 'file', size: 4096, modified: '2026-06-20' },
          { name: 'game.hpp', type: 'file', size: 2048, modified: '2026-06-18' },
          { name: 'CMakeLists.txt', type: 'file', size: 256, modified: '2026-06-18' },
        ],
      },
      {
        name: 'website', type: 'dir', modified: '2026-06-15',
        children: [
          { name: 'index.html', type: 'file', size: 8192, modified: '2026-06-15' },
          { name: 'style.css', type: 'file', size: 4096, modified: '2026-06-14' },
          { name: 'app.js', type: 'file', size: 12000, modified: '2026-06-13' },
        ],
      },
    ],
  },
  { name: '.bashrc', type: 'file', size: 1024, modified: '2026-01-01' },
  { name: '.gitconfig', type: 'file', size: 256, modified: '2026-01-01' },
]

function formatSize(bytes) {
  if (bytes >= 1e9) return (bytes / 1e9).toFixed(1) + ' GB'
  if (bytes >= 1e6) return (bytes / 1e6).toFixed(1) + ' MB'
  if (bytes >= 1000) return (bytes / 1000).toFixed(0) + ' KB'
  return bytes + ' B'
}

function formatDate(dateStr) {
  return dateStr
}

function getIcon(item) {
  if (item.type === 'dir') return '📁'
  const ext = item.name.split('.').pop().toLowerCase()
  const map = {
    pdf: '📄', txt: '📝', md: '📝', csv: '📊',
    jpg: '🖼️', jpeg: '🖼️', png: '🖼️', gif: '🖼️', webp: '🖼️', svg: '🖼️',
    mp4: '🎬', avi: '🎬', mov: '🎬', mkv: '🎬',
    mp3: '🎵', wav: '🎵', flac: '🎵', m3u: '🎵',
    html: '🌐', css: '🎨', js: '⚡', ts: '⚡',
    cpp: '⚙️', hpp: '⚙️', c: '⚙️', h: '⚙️',
    py: '🐍', java: '☕', rs: '🦀',
    deb: '📦', tar: '📦', gz: '📦', zip: '📦', tar_gz: '📦',
    docx: '📋', doc: '📋',
    m3u: '📋',
    cmake: '⚙️', txt: '📝',
  }
  return map[ext] || '📄'
}

const ROOT_PATH = '/'

export { tree, formatSize, formatDate, getIcon, ROOT_PATH }

export function getNode(path) {
  if (path === ROOT_PATH || path === '' || path === '/home') return tree
  const parts = path.split('/').filter(Boolean)
  let node = tree
  for (const part of parts) {
    if (part === 'home') continue
    const found = node.find(n => n.name === part)
    if (!found || found.type !== 'dir') return null
    node = found.children
  }
  return node
}

export function getParentPath(path) {
  if (path === ROOT_PATH || path === '/home') return ROOT_PATH
  const parts = path.split('/').filter(Boolean)
  parts.pop()
  return parts.length ? '/' + parts.join('/') : ROOT_PATH
}

export function getNodeDirname(path) {
  if (path === ROOT_PATH || path === '/home') return 'home'
  const parts = path.split('/').filter(Boolean)
  return parts[parts.length - 1]
}
