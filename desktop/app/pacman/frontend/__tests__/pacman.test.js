import { describe, it, expect, vi } from 'vitest'
import { mount } from '@vue/test-utils'

let onMsgCb = null
vi.mock('../../../../src/services/channel.js', () => ({
  createChannel: vi.fn(() => ({
    send: vi.fn(), onOpen: vi.fn(fn => fn()), onMessage: vi.fn(fn => { onMsgCb = fn }),
    onError: vi.fn(), onReconnecting: vi.fn(), onClose: vi.fn(), close: vi.fn(),
  })),
}))
const fireMsg = (d) => onMsgCb && onMsgCb(d)

import Game from '../index.vue'
const m = (p) => mount(Game, { props: p })

describe('Pacman', () => {
  it('游戏名称', () => { expect(m({gameType:'pacman'}).find('h1').text()).toBe('吃豆豆') })
  it('收到状态渲染网格', async () => {
    const w = m({gameType:'pacman'}); fireMsg({type:'snake',grid:'@ *\n* @',score:5,over:false}); await w.vm.$nextTick()
    expect(w.find('.grid-wrapper').exists()).toBe(true)
  })
  it('Game Over', async () => {
    const w = m({gameType:'pacman'}); fireMsg({type:'snake',grid:' \n$',score:10,over:true}); await w.vm.$nextTick()
    expect(w.text()).toContain('Game Over')
  })
  it('错误', async () => {
    const w = m({gameType:'pacman'}); fireMsg({type:'error',msg:'fail'}); await w.vm.$nextTick()
    expect(w.find('.error-msg').text()).toBe('fail')
  })
})
