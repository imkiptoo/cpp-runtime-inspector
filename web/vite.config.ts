import { sveltekit } from '@sveltejs/kit/vite';
import tailwindcss from '@tailwindcss/vite';
import { defineConfig } from 'vite';

// Backend port — keep in sync with the justfile (`just run backend`, default 8090).
// Override with BACKEND_PORT=... when running `npm run dev`.
const backendTarget = `http://localhost:${process.env.BACKEND_PORT ?? '8090'}`;

export default defineConfig({
	plugins: [tailwindcss(), sveltekit()],
	server: {
		proxy: {
			'/trace': {
				target: backendTarget,
				changeOrigin: true
			},
			'/health': {
				target: backendTarget,
				changeOrigin: true
			}
		}
	},
	optimizeDeps: {
		include: ['elkjs']
	},
	build: {
		target: 'esnext'
	}
});
