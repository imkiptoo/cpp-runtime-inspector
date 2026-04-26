<script lang="ts">
	import type { EncodedValue } from '$lib/trace/schema';
	import { isPointerLike, isCAddress, getPointerType } from '$lib/trace/schema';
	import { getHoverContext } from '$lib/contexts/hover.svelte';
	import ValueChip from './ValueChip.svelte';

	interface Props {
		name: string;
		value: EncodedValue;
		size?: number;
		showType?: boolean;
	}

	let { name, value, size, showType = false }: Props = $props();

	const hover = getHoverContext();

	// Get type label for display
	const typeLabel = $derived.by(() => {
		if (isPointerLike(value)) {
			if (isCAddress(value)) {
				return getPointerType(value) || 'pointer';
			}
			return 'pointer';
		}
		if (typeof value === 'number') {
			return Number.isInteger(value) ? 'int' : 'double';
		}
		if (typeof value === 'boolean') return 'bool';
		if (typeof value === 'string') {
			if (value.length === 1) return 'char';
			return 'string';
		}
		return null;
	});
</script>

<div
	class="grid grid-cols-[minmax(0,1fr)_auto] items-center gap-1.5 border-t border-islands-100 dark:border-islands-700/50 py-1.5 font-mono text-[12px] first:border-t-0"
	data-testid="local-{name}"
>
	<!-- Name : type -->
	<div class="flex min-w-0 items-baseline gap-1 truncate">
		<span class="text-islands-800 dark:text-islands-200">{name}</span>
		{#if showType && typeLabel}
			<span class="text-islands-400 dark:text-islands-500">:</span>
			<span class="text-islands-400 dark:text-islands-500">{typeLabel}</span>
		{/if}
		{#if size !== undefined}
			<span class="text-[11px] text-islands-400 dark:text-islands-500 ml-1">({size}B)</span>
		{/if}
	</div>

	<!-- Value chip -->
	<ValueChip {value} />
</div>
