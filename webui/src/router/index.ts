import { createRouter, createWebHistory } from 'vue-router'
import api from '../api'

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      name: 'Home',
      component: () => import('../views/HomeView.vue'),
    },
    {
      path: '/login/:token?',
      name: 'Login',
      props: true,
      component: () => import('../views/LoginView.vue'),
    },
  ],
})

router.beforeEach(async (to) => {
  const auth = await api.isAuthenticated()
  if (!auth && to.name !== 'Login') {
    return { name: 'Login' }
  }
  if (auth && to.name === 'Login') {
    return { name: 'Home' }
  }
})

export default router
