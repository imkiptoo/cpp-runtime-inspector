<script lang="ts">
	import type { StackFrame, EncodedValue } from '$lib/trace/schema';
	import { isCRef, isCRefOffset, isCDangling, getHeapId } from '$lib/trace/schema';
	import VarTable from './VarTable.svelte';
	import PointerChip from './PointerChip.svelte';

	interface Props {
		frame: StackFrame;
		isTop?: boolean;
	}

	let { frame, isTop = false }: Props = $props();

	// Expanded by default if highlighted (active frame)
	let expanded = $state(true);
	let pinned = $state(false);

	// Actual expansion state considers both expanded and pinned
	const isExpanded = $derived(expanded || pinned);

	const hasLocals = $derived(frame.ordered_varnames.length > 0);

	// Parse argument names from ordered_varnames that exist in encoded_locals
	// These are the function parameters shown in the signature
	const argNames = $derived(
		frame.ordered_varnames.filter(name => name in frame.encoded_locals)
	);

	// Extract heap pointer targets from locals (for collapsed state arrows)
	interface CollapsedPointer {
		heapId: number;
		isRef: boolean;
		offset?: number;
		isDangling?: boolean;
	}

	function extractPointers(value: EncodedValue): CollapsedPointer[] {
		const pointers: CollapsedPointer[] = [];
		if (isCRef(value)) {
			const heapId = getHeapId(value);
			if (heapId !== undefined) {
				pointers.push({ heapId, isRef: true });
			}
		} else if (isCRefOffset(value)) {
			pointers.push({ heapId: value[1], isRef: true, offset: value[2] });
		} else if (isCDangling(value)) {
			const heapId = getHeapId(value);
			if (heapId !== undefined) {
				pointers.push({ heapId, isRef: true, isDangling: true });
			}
		}
		return pointers;
	}

	const collapsedPointers = $derived.by(() => {
		const pointers: CollapsedPointer[] = [];
		const seen = new Set<number>();
		for (const name of frame.ordered_varnames) {
			const value = frame.encoded_locals[name];
			for (const ptr of extractPointers(value)) {
				if (!seen.has(ptr.heapId)) {
					seen.add(ptr.heapId);
					pointers.push(ptr);
				}
			}
		}
		return pointers;
	});

	// Build the frame CSS class
	const isHighlighted = $derived(frame.is_highlighted);
	const isZombie = $derived(frame.is_zombie);

	function toggleExpanded() {
		expanded = !expanded;
	}

	function togglePinned(e: MouseEvent) {
		e.stopPropagation();
		pinned = !pinned;
	}
</script>

<li
	class="overflow-hidden rounded-sm border transition-colors bg-white dark:bg-islands-800
		{isHighlighted ? 'border-brand-300 dark:border-brand-600 shadow-[inset_2px_0_0_var(--color-brand-500)]' : 'border-line'}
		{isZombie ? 'opacity-60' : ''}"
	data-testid="stack-frame"
	data-active={frame.is_highlighted || undefined}
	data-stack-frame-id={frame.frame_id}
	data-stack-expanded={isExpanded || undefined}
>
	<!-- Header row - clickable to toggle expand -->
	<div
		role="button"
		tabindex="0"
		onclick={toggleExpanded}
		onkeydown={(e) => {
			if (e.key === 'Enter' || e.key === ' ') {
				e.preventDefault();
				toggleExpanded();
			}
		}}
		class="flex w-full cursor-pointer items-center justify-between px-2.5 py-2 text-left hover:bg-islands-50 dark:hover:bg-islands-700/50 focus:outline-none focus-visible:bg-islands-50 dark:focus-visible:bg-islands-700/50"
	>
		<div class="flex min-w-0 items-center gap-2">
			<!-- Expand/collapse indicator -->
			<span
				aria-hidden
				class="inline-block w-2.5 font-mono text-[9px] text-islands-400 transition-transform duration-150 {isExpanded ? 'rotate-90 text-brand-500' : ''}"
			>
				▸
			</span>

			<!-- Function signature -->
			<span class="font-mono text-[13px] text-islands-900 dark:text-islands-100" data-testid="frame-name">
				<span class="font-medium">{frame.func_name}</span>
				<span class="text-islands-400 dark:text-islands-500">({argNames.join(', ')})</span>
			</span>

			{#if frame.is_zombie}
				<span class="px-1.5 py-0.5 text-[9px] font-medium uppercase tracking-wider bg-islands-100 text-islands-500 dark:bg-islands-700 dark:text-islands-400 rounded-sm">
					returned
				</span>
			{/if}

			{#if frame.is_ghost_dtor}
				<span class="px-1.5 py-0.5 text-[9px] font-medium uppercase tracking-wider bg-purple-100 text-purple-600 dark:bg-purple-900/30 dark:text-purple-300 rounded-sm">
					dtor
				</span>
			{/if}
		</div>

		<!-- Right side: collapsed pointers + pin button -->
		<div class="flex items-center gap-1.5 font-mono text-[11px] text-islands-400 dark:text-islands-500">
			<!-- Collapsed pointer chips (shown when frame is collapsed) -->
			{#if !isExpanded && collapsedPointers.length > 0}
				<div class="flex items-center gap-1">
					{#each collapsedPointers as ptr}
						<PointerChip
							heapId={ptr.heapId}
							isRef={ptr.isRef}
							offset={ptr.offset}
							isDangling={ptr.isDangling}
						/>
					{/each}
				</div>
			{/if}

			<!-- Pin button -->
			<button
				type="button"
				title={pinned ? 'Unpin' : 'Pin expanded'}
				onclick={togglePinned}
				class="rounded-sm p-0.5 hover:bg-islands-100 dark:hover:bg-islands-700 {pinned ? 'text-brand-500' : 'text-islands-400 hover:text-brand-500'}"
				data-testid="frame-pin"
				aria-pressed={pinned}
			>
				<svg class="w-3.5 h-3.5" fill="currentColor" viewBox="0 0 20 20">
					<path d="M10.75 4.75a.75.75 0 00-1.5 0v4.5h-4.5a.75.75 0 000 1.5h4.5v4.5a.75.75 0 001.5 0v-4.5h4.5a.75.75 0 000-1.5h-4.5v-4.5z" />
				</svg>
			</button>
		</div>
	</div>

	<!-- Locals section -->
	{#if isExpanded && hasLocals}
		<div class="border-t border-islands-100 dark:border-islands-700/50 px-2.5 pb-2.5 pt-1" data-testid="frame-locals">
			<VarTable
				names={frame.ordered_varnames}
				values={frame.encoded_locals}
				sizes={frame.local_sizes}
				types={frame.local_types}
				addresses={frame.local_addresses}
				showAddress={true}
			/>
		</div>
	{:else if isExpanded && !hasLocals}
		<div class="px-2.5 pb-2 text-[12px] text-islands-400 dark:text-islands-500 italic border-t border-islands-100 dark:border-islands-700/50 pt-1">
			No local variables
		</div>
	{/if}
</li>
