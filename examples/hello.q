chan<int> numbers;

fn producer() {
    numbers.send(1);
    numbers.send(2);
    numbers.send(3);
}

fn consumer() {
    let a: int = numbers.recv();
    let b: int = numbers.recv();
    let c: int = numbers.recv();
    print(a);
    print(b);
    print(c);
}

fn main() {
    go producer();
    go consumer();
    wait_all();
}