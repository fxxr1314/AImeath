import { bench, describe } from 'vitest'
import { ref } from 'vue'
import { styles } from '../config.js'

function makeGrid(w, h, chars) { let g = ''; for (let y = 0; y < h; y++) { for (let x = 0; x < w; x++) g += chars[Math.floor(Math.random() * chars.length)]; if (y < h - 1) g += '\n' } return g }
function cellStyle(c, s, cs) { const b = s[c] || { background: '#111' }; return { ...b, width: cs + 'px', height: cs + 'px' } }

describe('Gomoku 网格性能', () => {
  const g15 = makeGrid(15, 15, ['B', 'W', '.'])
  const cells15 = ref(g15.split('\n').filter(r => r.length > 0).join('').split(''))
  bench('15x15 cell 样式 (225 cells)', () => {
    for (let i = 0; i < cells15.value.length; i++) cellStyle(cells15.value[i], styles, 24)
  })
})

describe('Go 网格性能', () => {
  const g19 = makeGrid(19, 19, ['B', 'W', '.'])
  const cells19 = ref(g19.split('\n').filter(r => r.length > 0).join('').split(''))
  bench('19x19 cell 样式 (361 cells)', () => {
    for (let i = 0; i < cells19.value.length; i++) cellStyle(cells19.value[i], styles, 18)
  })
})
