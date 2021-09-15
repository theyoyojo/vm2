# vm
##### 2

Interesting demo:

```bash
$ make
$ ./app prog
```

This will create a file called bin.raw in the directory.
The file contains a binary representation of `prog`.
It looks something like this:

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

Execution example for `prog`:

```
$ ./app prog # OR ./app -b bin.raw after generating the a binary file
hello welkom 2 my  vm
[0]: ==
[1]: R0
[2]: 1
[3]: ++
[4]: R0
[5]: xxx
EXEC SYSTEM

REGISTERS:
[Display Binary Tape for @00001000 + 0004 lines]
| Emu. Address | Operation    | Argument one | Argument two | Argument three   |
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
>>>>> @00001000:   ( junk )   |   ( junk )   |   ( junk )   |   ( junk )   <<<<<
>>>>> @00001020:   ( junk )   |   ( junk )   |   ( junk )   |   ( junk )   <<<<<
>>>>> @00001040:   ( junk )   |   ( junk )   |   ( junk )   |   ( junk )   <<<<<
>>>>> @00001060:   ( junk )   | 3            | @c00000      | @400000      <<<<<
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\

CODE:
[Display Binary Tape for @00400000 + 0003 lines]
| Emu. Address | Operation    | Argument one | Argument two | Argument three   |
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
>>>>> @00400000: Move         | [R0]         | 1            |   ( junk )   <<<<<
>>>>> @00400020: Increment    | [R0]         |   ( junk )   |   ( junk )   <<<<<
>>>>> @00400040: Debug        |   ( junk )   |   ( junk )   |   ( junk )   <<<<<
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\

>> READ @00400000
>> READ @00400010
>> READ @00400008
>> WRITE  @00001000 = 1
>> READ @00001000
>> READ @00001008
>> READ @00001010
>> READ @00001018
>> READ @00001020
>> READ @00001028
>> READ @00001030
>> READ @00001038
>> READ @00001040
>> READ @00001048
>> READ @00001050
>> READ @00001058
>> READ @00001060
>> READ @00001068
>> READ @00001070
>> READ @00001078
>> READ @00400000
>> READ @00400008
>> READ @00400010
>> READ @00400018
[0] EXECUTE
>>>>> @00400000: Move         | [R0]         | 1            |   ( junk )   <<<<<
[Display Binary Tape for @00001000 + 0004 lines]
| Emu. Address | Operation    | Argument one | Argument two | Argument three   |
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
>>>>> @00001000: 1            |   ( junk )   |   ( junk )   |   ( junk )   <<<<<
>>>>> @00001020:   ( junk )   |   ( junk )   |   ( junk )   |   ( junk )   <<<<<
>>>>> @00001040:   ( junk )   |   ( junk )   |   ( junk )   |   ( junk )   <<<<<
>>>>> @00001060:   ( junk )   | 3            | @c00000      | @400000      <<<<<
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\

>> READ @00400020
>> READ @00400028
>> READ @00001000
>> READ @00400028
>> WRITE  @00001000 = 2
>> READ @00001000
>> READ @00001008
>> READ @00001010
>> READ @00001018
>> READ @00001020
>> READ @00001028
>> READ @00001030
>> READ @00001038
>> READ @00001040
>> READ @00001048
>> READ @00001050
>> READ @00001058
>> READ @00001060
>> READ @00001068
>> READ @00001070
>> READ @00001078
>> READ @00400020
>> READ @00400028
>> READ @00400030
>> READ @00400038
[1] EXECUTE
>>>>> @00400020: Increment    | [R0]         |   ( junk )   |   ( junk )   <<<<<
[Display Binary Tape for @00001000 + 0004 lines]
| Emu. Address | Operation    | Argument one | Argument two | Argument three   |
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
>>>>> @00001000: 2            |   ( junk )   |   ( junk )   |   ( junk )   <<<<<
>>>>> @00001020:   ( junk )   |   ( junk )   |   ( junk )   |   ( junk )   <<<<<
>>>>> @00001040:   ( junk )   |   ( junk )   |   ( junk )   |   ( junk )   <<<<<
>>>>> @00001060:   ( junk )   | 3            | @c00000      | @400000      <<<<<
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\

>> READ @00400040
... debug stub ...
MACHINE HALT
REGISTERS:
[Display Binary Tape for @00001000 + 0004 lines]
| Emu. Address | Operation    | Argument one | Argument two | Argument three   |
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
>>>>> @00001000: 2            |   ( junk )   |   ( junk )   |   ( junk )   <<<<<
>>>>> @00001020:   ( junk )   |   ( junk )   |   ( junk )   |   ( junk )   <<<<<
>>>>> @00001040:   ( junk )   |   ( junk )   |   ( junk )   |   ( junk )   <<<<<
>>>>> @00001060:   ( junk )   | 3            | @c00000      | @400000      <<<<<
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\

CODE:
[Display Binary Tape for @00400000 + 0003 lines]
| Emu. Address | Operation    | Argument one | Argument two | Argument three   |
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
>>>>> @00400000: Move         | [R0]         | 1            |   ( junk )   <<<<<
>>>>> @00400020: Increment    | [R0]         |   ( junk )   |   ( junk )   <<<<<
>>>>> @00400040: Debug        |   ( junk )   |   ( junk )   |   ( junk )   <<<<<
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\

```

This reproduces at least one bug:

```bash
$ make
$ ./app prog2
```

I'm going to implement C-like semantics, where the the first argument to a mutation must be an "l-value",
i.e a storage location, not one to be dereferenced, whereas a save/transfer operation will mutate the data
stored at the adress contained at that location instead of the extant data.

Floats and strings are broken for now.

The code works now too kinda.
