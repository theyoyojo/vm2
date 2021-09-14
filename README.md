# vm
##### 2

Interesting demo:

```bash
$ make
$ ./app prog
```

This will create a file called bin.raw in the directory.
The file contains a binary representation of `prog`:

```bash
$ hexdump bin.raw
0000000 0000 0000 0000 0000 0000 0000 0000 0000
*
0400000 0003 0000 0000 0100 1000 0000 0000 0200
0400010 0001 0000 0000 0300 0000 0000 0000 0000
0400020 0001 0000 0000 0100 1000 0000 0000 0200
0400030 0000 0000 0000 0000 0000 0000 0000 0000
0400040 0007 0000 0000 0100 0000 0000 0000 0000
0400050 0000 0000 0000 0000 0000 0000 0000 0000
*
1000000
```

Binary Tape representation for `prog`:

```
$ ./app prog # OR ./app -b bin.raw after generating the a binary file
[Display Binary Tape for @00400000 + 0003 lines]
| Emu. Address | Operation    | Argument one | Argument two | Argument three   |
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
>>>>> @00400000: Move         | [R0]         | 1            |   (no  op)   <<<<<
>>>>> @00400020: Increment    | [R0]         |   (no  op)   |   (no  op)   <<<<<
>>>>> @00400040: Debug        |   (no  op)   |   (no  op)   |   (no  op)   <<<<<
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
```
