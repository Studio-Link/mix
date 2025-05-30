import { createRouter, createWebHistory } from 'vue-router'
import api from '../api'
import config from '../config'

const router = createRouter({
  history: createWebHistory(config.base()),
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
    {
      path: '/social',
      name: 'Social',
      component: () => import('../views/SocialLoginView.vue'),
    },
    {
      path: '/fatal',
      name: 'FatalError',
      component: () => import('../views/FatalErrorView.vue'),
    },
  ],
})

interface Session {
    id: string
    auth: boolean
    user_id: string | null
    user_name: string
}

router.beforeEach(async (to) => {
  // deprecated session fallback
  let sess: Session = JSON.parse(window.localStorage.getItem('sess')!)
  if (sess) {
    console.log("session fallback");
    await api.session(sess.id)
    window.localStorage.removeItem('sess')
  }
  await api.connect(to.params.token)
  const auth = await api.isAuthenticated()
  if (!auth && to.name !== 'Login' && to.name !== 'Social') {
    return { name: 'Login' }
  }
  if (auth && to.name === 'Login') {
    return { name: 'Home' }
  }
})

export default router
