import { createRouter, createWebHistory } from 'vue-router'
import api from '../api'
import LoginView from '../views/LoginView.vue'

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      name: 'Home',
      component: () => import('../views/HomeView.vue'),
    },
    {
      path: '/login',
      name: 'Login',
      component: LoginView,
    },
  ],
})

router.beforeEach(async (to) => {
  if (!api.isAuthenticated() && to.name !== 'Login') {
    return { name: 'Login' }
  }
})

export default router
