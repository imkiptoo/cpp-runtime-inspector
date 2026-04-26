<script lang="ts">
	import type { EncodedValue } from '$lib/trace/schema';
	import { isPointerLike, isCAddress, getPointerType } from '$lib/trace/schema';
	import ValueChip from './ValueChip.svelte';

	interface Props {
		name: string;
		value: EncodedValue;
		showType?: boolean;
	}

	let { name, value, showType = false }: Props = $props();

	// Get type label for display
	const typeLabel = $derived.by(() => {
		if (!showType) return null;
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

<div class="grid grid-cols-[1fr_auto] items-center gap-2 border-t border-islands-100 dark:border-islands-700/50 px-2 py-1.5 first:border-t-0 font-mono text-[12px]">
	<!-- Name : type -->
	<div class="flex min-w-0 items-baseline gap-1">
		<span class="text-islands-800 dark:text-islands-200">{name}</span>
		{#if typeLabel}
			<span class="text-islands-400 dark:text-islands-500">:</span>
			<span class="text-islands-400 dark:text-islands-500">{typeLabel}</span>
		{/if}
	</div>

	<!-- Value chip -->
	<ValueChip {value} />
</div>
