import path from "node:path"
import tailwindcss from "@tailwindcss/vite"
import react from "@vitejs/plugin-react"
import { defineConfig } from "vite"
import { viteSingleFile } from "vite-plugin-singlefile"

// `npm run dev` forwards /api calls to the real station. Override with
//   set ESP32_HOST=http://192.168.1.50 && npm run dev
const ESP32_HOST = process.env.ESP32_HOST ?? "http://smartstation.local"

export default defineConfig({
  // Everything (JS, CSS, icons) is inlined into one index.html so the ESP32
  // can serve the whole dashboard from flash, even with no internet.
  plugins: [react(), tailwindcss(), viteSingleFile()],
  resolve: {
    alias: {
      "@": path.resolve(__dirname, "./src"),
    },
  },
  build: {
    target: "es2020",
    chunkSizeWarningLimit: 2000,
  },
  server: {
    proxy: {
      "/api": { target: ESP32_HOST, changeOrigin: true },
    },
  },
})
