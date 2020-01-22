# crepl

`crepl`, the abbreviation of C Read-Eval-Print-Loop, 
is an interactive shell for C like the one for python.

`crepl` reads expressions from `stdin`, and output result
to `stdout`. It only accepts the following two kind of inputs

* A valid C expression: 
`crepl` will evaluate the expression and output the numeric result.
* A valid C function with signature of `int f(..)`:
`crepl` will compile it for your later invoking.

Both of these inputs should be **in a line**.

## Build
    
    make

## Usage

    ./crepl

You can also use `./crepl < script` to execute a script.

## Example

See `scripts` for details.
