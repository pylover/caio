# caio (C AsyncIO)


C non-blocking IO without callbacks.

Yes, [Duff's device](https://en.wikipedia.org/wiki/Duff%27s_device) allows
simulating coroutine in C.

Here is a minimal `caio` coroutine:

```C
struct bar {
  int baz;
};


ASYNC
foo(struct caio_task *self, struct bar* state) {
  CAIO_BEGIN(self);

  /* Do something with state */
  state->baz++;

  CAIO_FINALLY(self);
}


int
main() {
  struct bar state = {0};
  return CAIO_FOREVER(foo, &state, 1);
}
```

Which translates to:
```C
void
foo(struct caio_task *self, struct bar* state) {
  switch (self->current->line) {
    case 0:

      /* Do something with state */
      state->baz++;

    case -1:
  }
  self->status = CAIO_TERMINATED
}
```


## Contribution

### Setup build environment

#### Install Dependencies
##### Build essentials
```bash
sudo apt install make cmake build-essential
```

##### Linter 
```bash
pip3 install prettyc
```
Or, in modern way
```bash
python3 -m venv ${HOME}/pyenv
${HOME}/pyenv/bin/pip install prettyc
cp ${HOME}/pyenv/bin/prettyc ${HOME}/bin
```
