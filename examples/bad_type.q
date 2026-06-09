fn multiply(a: int, b: int) -> int {
    return a + b;
}
fn wrongTypes(a: int, b: string) -> string {

}
fn main() {
    let result: int = multiply("hello", 7);
    let fault: string = wrongTypes(89, "world");

    print(result);
    print(fault);
}
