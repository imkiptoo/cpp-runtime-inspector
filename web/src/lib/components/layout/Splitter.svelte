<script lang="ts">
	interface Props {
		direction: 'horizontal' | 'vertical';
		onDrag: (delta: number) => void;
	}

	let { direction, onDrag }: Props = $props();

	let dragging = $state(false);
	let startPos = $state(0);

	function handleMouseDown(event: MouseEvent) {
		event.preventDefault();
		dragging = true;
		startPos = direction === 'horizontal' ? event.clientX : event.clientY;

		window.addEventListener('mousemove', handleMouseMove);
		window.addEventListener('mouseup', handleMouseUp);
	}

	function handleMouseMove(event: MouseEvent) {
		if (!dragging) return;

		const currentPos = direction === 'horizontal' ? event.clientX : event.clientY;
		const delta = currentPos - startPos;
		startPos = currentPos;
		onDrag(delta);
	}

	function handleMouseUp() {
		dragging = false;
		window.removeEventListener('mousemove', handleMouseMove);
		window.removeEventListener('mouseup', handleMouseUp);
	}
</script>

<!-- svelte-ignore a11y_no_noninteractive_element_interactions -->
<div
	class="splitter {direction === 'horizontal' ? 'w-1.5 cursor-col-resize' : 'h-1.5 cursor-row-resize'} {dragging ? 'dragging' : ''} flex-shrink-0"
	onmousedown={handleMouseDown}
	role="separator"
	aria-orientation={direction}
></div>
