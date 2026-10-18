import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

export default defineConfig({
  plugins: [react()],
  server: {
    watch: { ignored: ["**/backend-cpp/**", "**/node_modules/**", "**/data.sqlite*"] },
  },
  optimizeDeps: { entries: ["index.html"] },
});