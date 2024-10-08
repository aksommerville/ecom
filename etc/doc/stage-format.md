# Ecom Stage Format

Human-readable text in `src/www/stage/`.
File names don't matter but I'll use the stage id to preserve my sanity.
The actual id is assigned based on position of the `<stage>` tag in index.html.

That gets compiled to a tighter format during minification.
The process is duplicated for dev runs, in Stages.js.
In the compiled format, there's a one-character opcode followed by fixed arguments of one character each.
Count of arguments is hard-coded. Unknown opcodes are an error.
Most arguments will be an integer in 0..63, use the base64 alphabet. Plus "?" for 64, if we need it. So, base65 actually.
Spatial arguments, multiply by 4.

| Text                   | Compiled                         | Notes |
|------------------------|----------------------------------|-------|
| wall X Y W H           | w X Y W H                        | Text in pixels, compiled in 4 pixels. |
| hero X Y               | h X Y                            | Pixels. Horizontal center, vertical bottom. |
| bg RGB                 |                                  | Vestigial. Compiler drops it. |
| song NAME              |                                  | Vestigial. Compiler drops it. |
| platform X Y W DX DY   | p X Y W DX DY                    | Pixels. DX,DY bias by -32 compiled. Height always 8. |
| ladder X Y H           | l X Y H                          | Width always 16. |
| win X Y W H            | e X Y W H                        | Dot's center in range, she wins. |
