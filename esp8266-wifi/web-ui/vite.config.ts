import { defineConfig } from 'vite'
import preact from '@preact/preset-vite'
import tailwindcss from '@tailwindcss/vite'
export default defineConfig({
  plugins: [preact(), tailwindcss()],
  build: {
    target: 'es2015',
    minify: 'terser',
    cssMinify: true,
    terserOptions: {
      compress: {
        drop_console: true,
        drop_debugger: true,
        passes: 2,
      },
      mangle: {
        toplevel: true,
      },
    },
    rollupOptions: {
      output: {
        manualChunks: undefined,
        entryFileNames: 'app.[hash].js',
        chunkFileNames: 'app.[hash].js',
        assetFileNames: 'app.[hash].[ext]',
      }
    },
  },
  server: {
    proxy: {
      '/api': {
        target: 'http://192.168.4.1',
        changeOrigin: true,
      },
    },
  },
})
