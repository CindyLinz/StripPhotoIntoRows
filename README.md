# Purpose

Cut image into rows.

# Usage

- Take a look on cut\_line.c, see if you want to change the following arguments
  + #define GUESS\_ARG\_TIMES (100)
  + #define GUESS\_OFFSET\_SEP\_TIMES (100)
  + #define GUESS\_MAX\_ARG (M\_PI/10)
  + #define GUESSED\_MAX\_ROWS (200)
  + #define GUESSED\_MIN\_ROWS (50)
  + #define USE\_THREAD (4)

- execute

  ```
  make
  ```

- execute

  ```
  ./cut_line input.jpg
  ```

- Then you got cut\_line\_output.jpg and many cut\_strip\_???.jpg as a result

# Sample

The file 060.jpg is the sample input image.
And other jpeg files are some of the output images.
