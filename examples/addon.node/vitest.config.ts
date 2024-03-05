import { defineConfig } from 'vitest/config'

export default defineConfig({
  test: {
    include: ['__test__/**/*.spec.ts'],
  },
})
