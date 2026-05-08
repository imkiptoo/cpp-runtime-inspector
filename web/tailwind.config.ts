import type { Config } from 'tailwindcss';

export default {
	content: ['./src/**/*.{html,js,svelte,ts}'],
	darkMode: 'class',
	theme: {
		extend: {
			fontFamily: {
				sans: ['Inter', 'system-ui', '-apple-system', 'sans-serif'],
				mono: ['JetBrains Mono', 'Menlo', 'Monaco', 'Consolas', 'monospace']
			},
			colors: {
				// JetBrains Islands-inspired palette
				islands: {
					50: '#F7F8FA',
					100: '#EBECF0',
					200: '#DFE1E5',
					300: '#C9CCD6',
					400: '#9DA0A8',
					500: '#6E7179',
					600: '#4E5157',
					700: '#393B40',
					800: '#2B2D30',
					900: '#1E1F22',
					950: '#18191C'
				},
				// Islands accent blue
				brand: {
					50: '#E8F1FC',
					100: '#D1E3F9',
					200: '#A3C7F3',
					300: '#75ABEC',
					400: '#478FE6',
					500: '#3574F0',
					600: '#2B5FC7',
					700: '#224B9E',
					800: '#183775',
					900: '#0F234C',
					950: '#0A1833'
				},
				// Trace line highlight
				trace: {
					light: '#FEF9E7',
					dark: 'rgba(255, 193, 7, 0.15)'
				},
				// Warning/Orphan
				orphan: {
					light: '#FCD34D',
					dark: '#F59E0B'
				}
			},
			borderRadius: {
				'islands-sm': '6px',
				'islands': '8px',
				'islands-lg': '12px',
				'islands-xl': '16px'
			},
			boxShadow: {
				'islands-sm': '0 1px 2px rgba(0, 0, 0, 0.04)',
				'islands': '0 2px 8px rgba(0, 0, 0, 0.08)',
				'islands-lg': '0 4px 16px rgba(0, 0, 0, 0.12)'
			},
			animation: {
				'fade-in': 'fadeIn 150ms ease-out',
				'slide-up': 'slideUp 150ms ease-out',
				'scale-in': 'scaleIn 100ms ease-out'
			},
			keyframes: {
				fadeIn: {
					'0%': { opacity: '0' },
					'100%': { opacity: '1' }
				},
				slideUp: {
					'0%': { opacity: '0', transform: 'translateY(4px)' },
					'100%': { opacity: '1', transform: 'translateY(0)' }
				},
				scaleIn: {
					'0%': { opacity: '0', transform: 'scale(0.95)' },
					'100%': { opacity: '1', transform: 'scale(1)' }
				}
			},
			transitionDuration: {
				flip: '200ms'
			},
			transitionTimingFunction: {
				'islands': 'cubic-bezier(0.22, 1, 0.36, 1)'
			}
		}
	},
	plugins: []
} satisfies Config;
