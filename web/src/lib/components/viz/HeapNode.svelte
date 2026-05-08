<script lang="ts">
	import type { HeapObject, EncodedValue } from '$lib/trace/schema';
	import { getHoverContext } from '$lib/contexts/hover.svelte';
	import FieldRow from './FieldRow.svelte';
	import ValueChip from './ValueChip.svelte';

	interface Props {
		id: number;
		obj: HeapObject;
		address?: string;
		isOrphan?: boolean;
	}

	let { id, obj, address, isOrphan = false }: Props = $props();

	// Format address for display (use real address if available, fallback to ID-based)
	const displayAddress = $derived(
		address || `0x${id.toString(16).toUpperCase().padStart(4, '0')}`
	);

	const hover = getHoverContext();
	const isHighlighted = $derived(hover?.hoveredHeapId === id);

	const tag = $derived(obj[0]);
	const typeName = $derived(obj[1]);

	function handleMouseEnter() {
		hover?.setHoveredHeapId(id);
	}

	function handleMouseLeave() {
		hover?.setHoveredHeapId(null);
	}
</script>

<!-- Outer wrapper for layout tracking (FLIP animations target this) -->
<div
	data-heap-addr={id}
	data-orphan={isOrphan || undefined}
	data-highlighted={isHighlighted || undefined}
	data-testid="heap-node"
>
	<!-- Inner article for enter/exit animations -->
	<!-- svelte-ignore a11y_no_static_element_interactions -->
	<article
		data-heap-inner
		class="flex min-w-[140px] max-w-[280px] flex-col overflow-hidden rounded-sm border font-mono text-[12px] transition-colors duration-150 {
			isOrphan
				? 'border-dashed border-amber-400 dark:border-amber-600 bg-amber-50/50 dark:bg-amber-900/20'
				: isHighlighted
					? 'border-brand-400 dark:border-brand-500 bg-brand-50/50 dark:bg-brand-900/20 shadow-[0_0_0_1px_var(--color-brand-400)]'
					: 'border-line bg-white dark:bg-islands-800'
		}"
		onmouseenter={handleMouseEnter}
		onmouseleave={handleMouseLeave}
	>
		<!-- Header -->
		<header class="flex items-center justify-between border-b px-2 py-1.5 {
			isOrphan
				? 'border-amber-300 dark:border-amber-700 bg-amber-100/50 dark:bg-amber-900/30'
				: 'border-islands-100 dark:border-islands-700 bg-islands-50 dark:bg-islands-800/80'
		}">
			<span class="flex items-center gap-1.5 text-[11px] tracking-wide {
				isOrphan ? 'text-amber-600 dark:text-amber-400' : 'text-brand-600 dark:text-brand-400'
			}">
				{typeName}
				{#if isOrphan}
					<span
						title="No pointer from stack/globals reaches this block (potential leak)"
						class="rounded-sm border border-amber-400 dark:border-amber-600 bg-white dark:bg-islands-900 px-1 py-[0.5px] text-[9px] uppercase tracking-wider"
					>
						orphan
					</span>
				{/if}
			</span>
			<span class="text-[11px] text-islands-400 dark:text-islands-500 font-mono">{displayAddress}</span>
		</header>

		<!-- Content -->
		<div class="flex flex-col">
			{#if tag === 'HEAP_PRIMITIVE'}
				{@const primitiveValue = obj[2] as EncodedValue}
				<div class="px-2 py-1.5">
					<ValueChip value={primitiveValue} />
				</div>
			{:else if tag === 'HEAP_ARRAY'}
				{@const elements = obj[2] as EncodedValue[]}
				{#each elements.slice(0, 10) as elem, i}
					<FieldRow name="[{i}]" value={elem} showType={true} />
				{/each}
				{#if elements.length > 10}
					<div class="px-2 py-1 text-[11px] text-islands-400 dark:text-islands-500 italic border-t border-islands-100 dark:border-islands-700/50">
						... {elements.length - 10} more
					</div>
				{/if}
			{:else if tag === 'HEAP_STRUCT'}
				{@const fields = obj[2] as [string, EncodedValue][]}
				{@const dynamicType = obj[3]}
				{#if dynamicType && dynamicType !== typeName}
					<div class="px-2 py-1 text-[11px] text-purple-600 dark:text-purple-400 border-b border-islands-100 dark:border-islands-700/50">
						dynamic: {dynamicType}
					</div>
				{/if}
				{#each fields as [fieldName, fieldValue]}
					<FieldRow name={fieldName} value={fieldValue} showType={true} />
				{/each}
			{/if}
		</div>
	</article>
</div>
