import { describe, it, expect, vi, beforeAll } from 'vitest'
import { mount } from '@vue/test-utils'

// xterm mock
vi.mock('xterm', () => {
  class MockTerminal {
    constructor() { this.open = vi.fn(); this.loadAddon = vi.fn(); this.onData = vi.fn(); this.dispose = vi.fn(); this.write = vi.fn(); }
  }
  MockTerminal.prototype.rows = 24
  MockTerminal.prototype.cols = 80
  MockTerminal.prototype.options = {}
  return { Terminal: MockTerminal }
})
vi.mock('@xterm/addon-fit', () => ({ FitAddon: class { fit = vi.fn() } }))

// WebSocket mock (terminal creates new WebSocket directly)
class MockWS {
  constructor() { this.send = vi.fn(); this.close = vi.fn(); this.readyState = 1 }
  addEventListener() {}
  removeEventListener() {}
}
MockWS.OPEN = 1
globalThis.WebSocket = MockWS

import TerminalPage from '../index.vue'

describe('Terminal', () => {
  it('挂载不崩溃', () => {
    const w = mount(TerminalPage)
    expect(w.find('.term-container').exists()).toBe(true)
    w.unmount()
  })
})
