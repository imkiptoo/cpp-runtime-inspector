<script lang="ts">
	import type { EncodedValue } from '$lib/trace/schema';
	import {
		isCAddress,
		isCRef,
		isCRefOffset,
		isCDangling,
		getAddressHex,
		getPointerType,
		getMemoryRegion,
		getHeapId
	} from '$lib/trace/schema';
	import PointerChip from './PointerChip.svelte';

	interface Props {
		value: EncodedValue;
	}

	let { value }: Props = $props();

	// Format display value for non-pointer types
	function formatValue(val: EncodedValue): string {
		if (val === null) return 'null';
		if (val === undefined) return '?';
		if (typeof val === 'boolean') return val ? 'true' : 'false';
		if (typeof val === 'number') {
			if (!Number.isInteger(val)) {
				return val.toFixed(6).replace(/\.?0+$/, '');
			}
			return String(val);
		}
		if (typeof val === 'string') {
			if (val === '<UNINITIALIZED>') return '?';
			if (val.length === 1) return `'${val}'`;
			return `"${val}"`;
		}
		// For arrays and structs, show compact representation
		if (Array.isArray(val)) {
			const tag = val[0];
			if (tag === 'C_STRUCT') return `{...}`;
			if (tag === 'C_ARRAY') return `[${val[2]?.length ?? 0}]`;
			if (tag === 'C_UNION') return `{...}`;
		}
		return String(val);
	}
</script>

{#if isCAddress(value)}
	<!-- Direct address pointer -->
	<PointerChip
		address={getAddressHex(value)}
		type={getPointerType(value)}
		region={getMemoryRegion(value)}
	/>
{:else if isCRef(value)}
	<!-- Heap reference -->
	<PointerChip
		heapId={getHeapId(value)}
		isRef
	/>
{:else if isCRefOffset(value)}
	<!-- Heap reference with offset -->
	<PointerChip
		heapId={value[1]}
		offset={value[2]}
		isRef
	/>
{:else if isCDangling(value)}
	<!-- Dangling pointer -->
	<PointerChip
		heapId={getHeapId(value)}
		isDangling
	/>
{:else}
	<!-- Non-pointer primitive value -->
	<span class="max-w-[110px] truncate rounded-sm border border-line bg-islands-50 dark:bg-islands-800 px-1.5 py-[1px] text-[11.5px] text-islands-700 dark:text-islands-300">
		{formatValue(value)}
	</span>
{/if}
