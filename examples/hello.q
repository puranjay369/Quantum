fn task(id: int) {
    print(id);
}

fn main() {
    go task(1);
    go task(2);
    wait_all();
}