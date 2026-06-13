fn main() {
    let arr: heap<int> = heap_alloc(5);
    arr[0] = 10;
    arr[1] = 20;
    print(arr[0]);
    print(arr[1]);
}