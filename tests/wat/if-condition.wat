;;; TOOL: wat2wasm
(module
  (func 
    i32.const 0
    if (result i32)
      i32.const 1
    else
      i32.const 2
    end
    if 
      nop
    else
      nop
    end
  )
)