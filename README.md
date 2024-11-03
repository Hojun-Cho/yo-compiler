# yo-compiler

```c
package main

type Any struct {
	x,y int;
	arr [8]int;
};

fn main() int {
	arr := [8]int{};
	return nqueen(0, 0, arr[:]);
};

fn fib(n int) int{
	if n <= 1 {
		return n;
	};
	return fib(n-1) + fib(n-2);
};

fn nqueen(r,ans int, arr []int) int{
	if r == len(arr) {
		return ans + 1;
	};
	for c:=0; c < len(arr); c=c+1{
		arr[r] = c;
		if canput(r, arr) {
			ans = nqueen(r+1, ans, arr);
		};
	};
	return ans;
};

fn canput(r int, arr []int) bool {
	for i:=0; i < r; i=i+1 {
		if arr[r] == arr[i] || abs(arr[r] - arr[i]) == abs(r - i) {
			return false;
		};
	};
	return true;
};

fn abs(x int) int {
	if x < 0 {
		return x * -1;
	};
	return x;
};

```
