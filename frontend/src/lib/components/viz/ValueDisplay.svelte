<script lang="ts">
	import type { EncodedValue } from '$lib/trace/schema';
	import {
		isCAddress,
		isCRef,
		isCRefOffset,
		isCDangling,
		isCStruct,
		isCArray,
		isCUnion,
		getAddressHex,
		getPointerType,
		getMemoryRegion,
		getHeapId
	} from '$lib/trace/schema';
	import { getHoverContext } from '$lib/contexts/hover.svelte';
	import PointerChip from './PointerChip.svelte';
	// Self import for recursive rendering
	import ValueDisplay from './ValueDisplay.svelte';

	interface Props {
		value: EncodedValue;
		compact?: boolean;
	}

	let { value, compact = false }: Props = $props();

	const hover = getHoverContext();

	function formatPrimitive(val: EncodedValue): string {
		if (val === null) return 'null';
		if (typeof val === 'boolean') return val ? 'true' : 'false';
		if (typeof val === 'number') {
			// Format floats nicely
			if (!Number.isInteger(val)) {
				return val.toFixed(6).replace(/\.?0+$/, '');
			}
			return String(val);
		}
		if (typeof val === 'string') {
			// Check if it's a char (single character)
			if (val.length === 1) {
				return `'${val}'`;
			}
			// Escape string for display
			return `"${val.replace(/"/g, '\\"')}"`;
		}
		return String(val);
	}

	function getValueClass(val: EncodedValue): string {
		if (val === null) return 'value-null';
		if (typeof val === 'boolean') return 'value-bool';
		if (typeof val === 'number') {
			return Number.isInteger(val) ? 'value-int' : 'value-float';
		}
		if (typeof val === 'string') {
			return val.length === 1 ? 'value-char' : 'value-string';
		}
		return '';
	}
</script>

{#if value === null}
	<span class="value-null">null</span>
{:else if typeof value === 'boolean'}
	<span class="value-bool">{value ? 'true' : 'false'}</span>
{:else if typeof value === 'number'}
	<span class={getValueClass(value)}>{formatPrimitive(value)}</span>
{:else if typeof value === 'string'}
	<span class={getValueClass(value)}>{formatPrimitive(value)}</span>
{:else if isCAddress(value)}
	<PointerChip
		address={getAddressHex(value)}
		type={getPointerType(value)}
		region={getMemoryRegion(value)}
	/>
{:else if isCRef(value)}
	<PointerChip
		heapId={getHeapId(value)}
		isRef
	/>
{:else if isCRefOffset(value)}
	<PointerChip
		heapId={value[1]}
		offset={value[2]}
		isRef
	/>
{:else if isCDangling(value)}
	<PointerChip
		heapId={getHeapId(value)}
		isDangling
	/>
{:else if isCStruct(value)}
	{@const typeName = value[1]}
	{@const fields = value[2]}
	{@const dynamicType = value[3]}
	{#if compact}
		<span class="text-zinc-500">{dynamicType || typeName}{'{...}'}</span>
	{:else}
		<div class="inline-block align-top">
			<span class="text-zinc-500 dark:text-zinc-400">{dynamicType || typeName}</span>
			<span class="text-zinc-400">{'{'}</span>
			<div class="pl-4">
				{#each Object.entries(fields) as [fieldName, fieldValue]}
					<div class="flex items-start gap-1">
						<span class="text-zinc-500">{fieldName}:</span>
						<ValueDisplay value={fieldValue} compact />
					</div>
				{/each}
			</div>
			<span class="text-zinc-400">{'}'}</span>
		</div>
	{/if}
{:else if isCArray(value)}
	{@const elemType = value[1]}
	{@const elements = value[2]}
	{#if compact || elements.length > 5}
		<span class="text-zinc-500">{elemType}[{elements.length}]</span>
	{:else}
		<span class="text-zinc-400">[</span>
		{#each elements as elem, i}
			{#if i > 0}<span class="text-zinc-400">, </span>{/if}
			<ValueDisplay value={elem} compact />
		{/each}
		<span class="text-zinc-400">]</span>
	{/if}
{:else if isCUnion(value)}
	{@const typeName = value[1]}
	{@const fields = value[2]}
	<span class="text-zinc-500">{typeName}</span>
	<span class="text-zinc-400">{'{'}</span>
	{#each Object.entries(fields).filter(([k]) => k !== '__raw') as [fieldName, fieldValue], i}
		{#if i > 0}<span class="text-zinc-400">, </span>{/if}
		<span class="text-zinc-500">{fieldName}:</span>
		<ValueDisplay value={fieldValue} compact />
	{/each}
	<span class="text-zinc-400">{'}'}</span>
{:else}
	<span class="text-zinc-500">{JSON.stringify(value)}</span>
{/if}
