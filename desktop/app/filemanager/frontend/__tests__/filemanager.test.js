import { describe, it, expect, vi } from 'vitest'
import { mount } from '@vue/test-utils'

// FileManager 通过 filemgrChannel.js 创建 WebSocket
class MockWS {
  constructor() { this.send = vi.fn(); this.close = vi.fn() }
  addEventListener() {}
  removeEventListener() {}
}
globalThis.WebSocket = MockWS
MockWS.OPEN = 1

import FileManager from '../index.vue'

describe('FileManager', () => {
  it('挂载显示标题', () => {
    const w = mount(FileManager)
    expect(w.find('.fm-title').text()).toBe('文件管理器')
  })

  it('挂载不报错', () => {
    const w = mount(FileManager)
    w.unmount()
  })
})
