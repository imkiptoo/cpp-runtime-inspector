<script lang="ts">
	import type { MemoryRegion } from '$lib/trace/schema';
	import { getHoverContext } from '$lib/contexts/hover.svelte';
	import { getHeapAddressesContext } from '$lib/contexts/heapAddresses.svelte';

	interface Props {
		address?: string;
		type?: string;
		region?: MemoryRegion;
		heapId?: number;
		offset?: number;
		isRef?: boolean;
		isDangling?: boolean;
	}

	let { address, type, region, heapId, offset, isRef = false, isDangling = false }: Props = $props();

	const hover = getHoverContext();
	const heapAddresses = getHeapAddressesContext();

	// Get real address from context if available
	const realAddress = $derived(
		heapId !== undefined ? heapAddresses?.getAddress(heapId) : undefined
	);

	const isNull = $derived(region === 'null' || address === '0x0' || address === 'nullptr');

	// Compute the target for hover highlighting
	const target = $derived(
		heapId !== undefined ? String(heapId) : address ?? null
	);

	const isHighlighted = $derived(
		target !== null && (
			(heapId !== undefined && hover?.hoveredHeapId === heapId) ||
			(address !== undefined && hover?.hoveredAddress === address)
		)
	);

	function handleMouseEnter() {
		if (isNull) return; // Don't highlight nullptr
		if (heapId !== undefined) {
			hover?.setHoveredHeapId(heapId);
		} else if (address) {
			hover?.setHoveredAddress(address);
		}
	}

	function handleMouseLeave() {
		hover?.setHoveredHeapId(null);
		hover?.setHoveredAddress(null);
	}

	function formatHeapAddress(id: number): string {
		// Use real address if available, otherwise fallback to ID-based format
		if (realAddress) {
			return realAddress;
		}
		return `0x${id.toString(16).toUpperCase().padStart(4, '0')}`;
	}

	function formatDisplay(): string {
		if (isDangling) {
			return `dangling`;
		}
		if (isRef && heapId !== undefined) {
			const addr = formatHeapAddress(heapId);
			if (offset !== undefined && offset > 0) {
				return `${addr}+${offset}`;
			}
			return addr;
		}
		if (address) {
			if (isNull) {
				return 'nullptr';
			}
			// Shorten address for display
			const shortAddr = address.length > 10 ? '...' + address.slice(-6) : address;
			return shortAddr;
		}
		return '???';
	}
</script>

<!-- svelte-ignore a11y_no_static_element_interactions -->
{#if isNull}
	<!-- Nullptr with strikethrough -->
	<span
		class="relative max-w-[110px] truncate rounded-sm border border-line bg-islands-50 dark:bg-islands-800 px-1.5 py-[1px] text-[11px] text-islands-400 dark:text-islands-500"
		data-ptr-target="null"
		data-ptr-kind={isRef ? 'ref' : 'pointer'}
	>
		nullptr
		<span
			aria-hidden
			class="pointer-events-none absolute left-1 right-1 top-1/2 h-[1px] -rotate-[12deg] bg-islands-400/70 dark:bg-islands-500/70"
		></span>
	</span>
{:else if isDangling}
	<!-- Dangling pointer -->
	<span
		class="max-w-[110px] truncate rounded-sm border border-red-300 dark:border-red-700 bg-red-50 dark:bg-red-900/30 px-1.5 py-[1px] text-[11px] text-red-600 dark:text-red-400"
		data-ptr-target={heapId}
		data-ptr-kind="dangling"
	>
		{formatDisplay()}
	</span>
{:else}
	<!-- Normal pointer with hover interaction -->
	<span
		class="max-w-[110px] truncate rounded-sm border px-1.5 py-[1px] text-[11px] transition-colors duration-150 {
			isHighlighted
				? 'border-brand-400 bg-brand-50 dark:bg-brand-900/30 text-brand-600 dark:text-brand-400'
				: 'border-brand-200 dark:border-brand-700 bg-islands-50 dark:bg-islands-800 text-brand-600 dark:text-brand-400'
		}"
		data-ptr-target={target}
		data-ptr-kind={isRef ? 'ref' : 'pointer'}
		data-highlighted={isHighlighted || undefined}
		onmouseenter={handleMouseEnter}
		onmouseleave={handleMouseLeave}
	>
		{formatDisplay()}
	</span>
{/if}
