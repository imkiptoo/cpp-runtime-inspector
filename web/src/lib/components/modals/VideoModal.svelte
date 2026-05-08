<script lang="ts">
	import Modal from './Modal.svelte';

	interface Props {
		open: boolean;
		onClose: () => void;
	}

	let { open, onClose }: Props = $props();

	// YouTube video ID - replace with actual video ID
	const videoId = 'dQw4w9WgXcQ'; // Placeholder - update with actual video
</script>

<Modal {open} {onClose} title="What & Why" maxWidth="5xl">
	{#snippet children()}
		<div class="p-3">
			<div class="aspect-video w-full rounded-md overflow-hidden bg-islands-900">
				<iframe
					src="https://www.youtube.com/embed/{videoId}"
					title="C++ Runtime Inspector - What & Why"
					frameborder="0"
					allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share"
					referrerpolicy="strict-origin-when-cross-origin"
					allowfullscreen
					class="w-full h-full"
				></iframe>
			</div>
		</div>
	{/snippet}
</Modal>
