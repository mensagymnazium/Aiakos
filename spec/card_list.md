# Card list file format

A card list consists of several card numbers, each on separate lines:

```
fd63afe2
4cce9cb2

9e022189
302b9a42
50ae2555
```

Empty lines are permitted.

## Numbers

Card numbers can be expressed as either a decimal or hexadecimal number.

### Hexadecimal numbers

Hexadecimal numbers are expressed as one to eight hexadecimal digit without any prefix, like this:

```
78ABCDEF
7af561d2
1e78
08A1D3CE
```

### Decimal numbers

Decimal numbers use `'` as a prefix, followed by one or more digits:

```
'12345678
'1375
'4294967295
'001652
```

### Separators

For readability, numbers may contain any number of separator characters (`-`, `_` or `.`) or whitespace:

```
BE-EF-F0-0D
c2d.7ef
'1 158 943
'59_305_643
'  2 459 661 279
```

## Comments

To help keep track of which numbers are which, you can use comments. A comment starts with a `;` and continues until the end of the line:

```
; Management
80ce622a ; Chloe Stewart
21b53368 ; Ellie Williams

; Teachers
4eb561c1 ; Jack Martinez
164cd401 ; Jessica Smith
5b612a4d ; William Campbell
f5d5c4ba ; Thomas Perez
```
