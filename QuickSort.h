#pragma once

/*
 * Function to swap two elements.
 */
template<typename T>
inline void Swap(T& a, T& b)
{
	T t = a;
	a = b;
	b = t;
}

/*
 * A function that finds the middle of the
 * values a, b, c and return that value.
 */
template<typename T>
inline T& MedianOfThree(T& a, T& b, T& c)
{
	if (a < b && b < c) // a < b < c
		return b;

	if (a < c && c <= b) // a < c <= b
		return c;

	if (b <= a && a < c) // b <= a < c
		return a;

	if (b < c && c <= a) // b < c <= a
		return c;

	if (c <= a && a < b) // c <= a < b
		return a;

	if (c <= b && b <= a) // c <= b <= a
		return b;

	return b; // For dummier compilers. In real this case is impossible.
}

/*
 * Partition the array using the last element as the pivot.
 * arr[]: Array to be sorted.
 * low: Starting index.
 * high: Ending index.
 */
template<typename T>
inline int QuickSortPartition(T arr[], int low, int high)
{
	// Choosing the pivot
	const T pivot = arr[high];

	// Index of smaller element and indicates
	// the right position of pivot found so far
	int i = (low - 1);

	for (int j = low; j < high; j++) {
		// If current element is smaller than the pivot
		if (arr[j] < pivot) // Could be <=, then pivot values would go "left" of final i+1.
		{
			// Increment index of smaller element
			i++;
			Swap(arr[i], arr[j]);
		}
	}
	Swap(arr[i + 1], arr[high]); // j = high
	return (i + 1);
}

/*
 * The main function that implements QuickSort.
 * arr[]: Array to be sorted.
 * low: Starting index.
 * high: Ending index.
 */
template<typename T>
inline void QuickSort(T arr[], const int low, const int high)
{
	if (low < high) {
		// Pivot must be in high index so swap it with median of three index.
		T& pivot = MedianOfThree(arr[low], arr[low + (high - low) / 2], arr[high]);
		Swap(pivot, arr[high]);

		// pi is partitioning index, arr[pi] is now at right place.
		const int pi = QuickSortPartition(arr, low, high);

		// Separately sort elements before partition and after partition.
		QuickSort(arr, low, pi - 1);
		QuickSort(arr, pi + 1, high);
	}
}
