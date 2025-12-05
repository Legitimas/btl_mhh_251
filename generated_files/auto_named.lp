Minimize
 obj: 0

Subject To
 c_state_0: x0 + 1 s1 - 1 s2 = 1
 c_state_1: x1 - 1 s1 + 1 s2 = 0
 c_state_2: x2 - 1 s0 + 1 s3 = 1
 c_state_3: x3 + 1 s0 - 1 s3 = 0
 c_state_4: x4 - 1 s0 + 1 s1 - 1 s2 + 1 s3 = 1
 c_dead_0: x3 <= 0
 c_dead_1: x0 + x4 <= 1
 c_dead_2: x1 <= 0
 c_dead_3: x2 + x4 <= 1

Bounds
 s0 >= 0
 s1 >= 0
 s2 >= 0
 s3 >= 0

Binaries
 x0
 x1
 x2
 x3
 x4
Generals
 s0
 s1
 s2
 s3
End
