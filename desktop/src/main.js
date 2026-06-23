import { createApp } from 'vue'
import { createRouter, createWebHashHistory } from 'vue-router'
import App from './App.vue'
import HomePage from './views/HomePage.vue'
import { APPS } from './config/games.js'

const routes = [
  { path: '/', component: HomePage },
  ...Object.keys(APPS).map(key => ({
    path: `/${key}`,
    component: APPS[key].page,
    props: { gameType: key },
  })),
]

const router = createRouter({
  history: createWebHashHistory(),
  routes,
})

createApp(App).use(router).mount('#app')
