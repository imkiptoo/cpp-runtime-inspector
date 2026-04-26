<script lang="ts">
	import { onMount } from 'svelte';

	interface Props {
		open: boolean;
		onClose: () => void;
		title?: string;
		maxWidth?: 'md' | 'lg' | 'xl' | '2xl' | '3xl' | '4xl' | '5xl';
		children: import('svelte').Snippet;
	}

	let { open, onClose, title, maxWidth = '3xl', children }: Props = $props();

	const maxWidthClass: Record<string, string> = {
		'md': 'max-w-md',
		'lg': 'max-w-lg',
		'xl': 'max-w-xl',
		'2xl': 'max-w-2xl',
		'3xl': 'max-w-3xl',
		'4xl': 'max-w-4xl',
		'5xl': 'max-w-5xl'
	};

	let dialogRef: HTMLDialogElement | undefined = $state();

	function handleKeydown(event: KeyboardEvent) {
		if (event.key === 'Escape') {
			event.preventDefault();
			onClose();
		}
	}

	function handleBackdropClick(event: MouseEvent) {
		if (event.target === dialogRef) {
			onClose();
		}
	}

	$effect(() => {
		if (open && dialogRef) {
			dialogRef.showModal();
		} else if (!open && dialogRef) {
			dialogRef.close();
		}
	});

	onMount(() => {
		return () => {
			if (dialogRef?.open) {
				dialogRef.close();
			}
		};
	});
</script>

<dialog
	bind:this={dialogRef}
	class="p-0 m-auto w-full bg-transparent backdrop:bg-islands-900/60 backdrop:backdrop-blur-sm {maxWidthClass[maxWidth]}"
	onkeydown={handleKeydown}
	onclick={handleBackdropClick}
>
	<div class="modal-content m-3 animate-scale-in overflow-hidden" onclick={(e) => e.stopPropagation()}>
		{#if title}
			<div class="flex items-center justify-between px-3 py-1.5 border-b border-line">
				<h2 class="text-base font-semibold text-islands-900 dark:text-islands-100">{title}</h2>
				<button
					class="py-1.5 pl-1.5 text-islands-400 hover:text-islands-600 cursor-pointer transition-colors"
					onclick={onClose}
					aria-label="Close modal"
				>
					<svg class="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
						<path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M6 18L18 6M6 6l12 12" />
					</svg>
				</button>
			</div>
		{/if}

		{@render children()}
	</div>
</dialog>
