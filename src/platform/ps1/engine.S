/*
 * Cheat Device PS1 Engine
 * Based on Xploder V4 engine. Modified to use GameShark code formats.
 */

#define CODELIST_START      0x8000F800

    .text

    .set noat // Disable warnings about using $at register
    .set noreorder

EngineHandler:
    // Only execute the engine if the ReturnFromException() BIOS function is
    // being called.
    li  $t0, 0x17
    bne $t1, $t0, JumpToBFunctionHandler
    nop

    // Save registers to the stack
    addiu   $sp, -0x30
    sw      $at, 0x0c($sp)
    sw      $ra, 0x10($sp)
    sw      $a0, 0x14($sp)
    sw      $a1, 0x18($sp)
    sw      $a2, 0x1c($sp)
    sw      $a3, 0x20($sp)
    sw      $v0, 0x24($sp)
    sw      $v1, 0x28($sp)

    la      $a0, CODELIST_START

HandleCode:
    lw      $a1, 0($a0)
    nop
    beqz    $a1, EngineExit
    nop

    lw      $a2, 4($a0)         // a2 = code value
    nop

    li      $v1, 0x0fffffff
    srl     $v0, $a1, 28        // v0 = code type
    and     $a1, $a1, $v1       // a1 = code address

    /*
     * Available to all code handlers:
     * a0: code list address
     * a1: code address
     * a2: code value
     * v0: code type
     * v1: mask to remove first octet using and
     * a3: (free)
     * t0: (free)
     * t1: (free)
     */

CheckType_F:
    li  $t0, 0xf
    bne $v0, $t0, CheckType_B

HandleCode_F:
    /*
     * 16-bit Conditional Master Trigger
     * 
     * F0aaaaaa 0000vvvv
     * 
     * Executes all following codes if the 16-bit value at @a is equal to @v.
     */
    lhu $t0, 0($a1)
    nop
    beq $t0, $a2, NextCode
    nop
    b   EngineExit
    nop

CheckType_B:
    li  $t0, 0xb
    bne $t0, $v0, CheckType_D

HandleCode_B:
    /*
     * Constant Serial Write
     *
     * Bnnnssss iiii
     * 10aaaaaa vvvv
     *
     * Starting at address @a, write the 16-bit value @v to @n addresses. After
     * each iteration the address is incremented by @s and the value is
     * incremented by @i.
     *
     * TODO: Currently only works if the destination address is 16-bit alligned.
     * Update to write 1 byte at a time
     */
    srl $t0, $a1, 16    // t0 = count
    lw $a2, 0x8($a0)
    lw $v0, 0xc($a0)    // v0 = value
    and $a2, $v1, $a2   // a2 = destination
    lh $a1, 0($a0)      // a1 = addr_increment
    lh $v1, 4($a0)      // v1 = value_increment
1:
    beqz $t0, NextCode     // if(!count) NextCode()
    nop
    sh $v0, 0($a2)         // *destination = value
    addiu $t0, -1          // count--;
    addu $a2, $a1, $a2     // destination += addr_increment
    b 1b                   // goto 1
    addu $v0, $v1, $v0     // value += value_increment (delay slot)

CheckType_D:
    li     $t0, 0xd
    bne    $t0, $v0, CheckType_3

HandleCode_D:
    /*
     * 16-bit Conditional
     *
     * Dtaaaaaa vvvv
     *
     * Compares the value at address @a to the 16-bit value @v and executes the
     * next code if the test condition @t is true. Values for @t are:
     *
     *   0: Equal
     *   1: Not Equal
     *   2: Less Than
     *   3: Greater Than
     */
    lhu     $t0, 0($a1)         // t0 = actual value, a2 = expected value
    srl     $a3, $a1, 24        // a3 = test
    
    // ==
    beqz    $a3, 3f
    subu    $v0, $t0, $a2

    // !=
    li      $t1, 1
    beq     $a3, $t1, 4f

    // <
    li      $t1, 2
    beq     $a3, $t1, 4f
    sltu    $v0, $t0, $a2

    // >
    li      $t1, 3
    beq     $a3, $t1, 4f
    sgtu    $v0, $t0, $a2

3: // Execute next code if v0 == 0
    beqz    $v0, NextCode
    nop
    b       NextCode
    addiu   $a0, 8

4: // Execute next code if v0 != 0
    bnez    $v0, NextCode
    nop
    b       NextCode
    addiu   $a0, 8


CheckType_3:
    li  $t0, 3
    bne $t0, $v0, CheckType_8
    nop

HandleCode_3:
    /*
     * 8-bit Constant Write
     *
     * 30aaaaaa 000vv
     *
     * Write the 8-bit value @v to address @a.
     */
    sb  $a2, 0($a1)

CheckType_8:
    li  $t0, 8
    bne $v0, $t0, CheckType_0
    nop

HandleCode_8:
    /*
     * 16-bit Constant Write
     *
     * 80aaaaaa vvvv
     *
     * Write the 16-bit value @v to address @a.
     */
    sh  $a2, 0($a1)
    nop

CheckType_0:
    bnez    $v0, CheckType_5
    nop

HandleCode_0:
    /*
     * 32-bit Constant Write
     *
     * 00aaaaaa vvvv
     *
     * Write the 32-bit value @v to address @a.
     */
    sw  $a2, 0($a1)

CheckType_5:
    li $t0, 5
    bne $t0, $v0, NextCode
    nop

HandleCode_5:
    /*
     * Constant Array Write
     * 
     * 50aaaaaa 0nnn
     * (n bytes)
     *
     * Write the following @n bytes to address @a.
     *
     * WARNING: Ensure that any code following the end of a type-5 code is
     * 32-bit aligned.
     */
    lh      $v0, 0($a2)
    addiu   $a2, 4
1:
    lb      $v1, 0($a2)     // read byte
    addiu   $a2, 1          // increment source address
    sb      $v1, 0($a1)     // write byte
    addiu   $v0, -1         // decrement counter
    bnez    $v0, 1b         // loop if counter != 0
    addiu   $a1, 1          // increment destination address
    b       NextCode
    nop

NextCode:
    b   HandleCode
    addiu  $a0, 0x8

EngineExit:
    // Restore registers from the stack
    lw  $at, 0x0c($sp)
    lw  $ra, 0x10($sp)
    lw  $a0, 0x14($sp)
    lw  $a1, 0x18($sp)
    lw  $a2, 0x1c($sp)
    lw  $a3, 0x20($sp)
    lw  $v0, 0x24($sp)
    lw  $v1, 0x28($sp)
    addiu   $sp, 0x30

JumpToBFunctionHandler:
    li  $t0, 0x5e0 // Real BIOS
    //li  $t0, 0xb60 // NO$PSX
    jr  $t0
    nop