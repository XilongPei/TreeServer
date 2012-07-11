/****************
 * ascii_7.C
 * ^-_-^  2000.8.10
 * ascii state table, for speed up the char compare
 ***************************************************************************/

// ' ' \t \n \r

char asc_asqlEscape[]= "\0"     //  0   0   
		   "\0"     //  1   1  
		   "\0"     //  2   2  
		   "\0"     //  3   3  
		   "\0"     //  4   4  
		   "\0"     //  5   5  
		   "\0"     //  6   6  
		   "\0"     //  7   7  
		   "\0"     //  8   8  
		   "\0"     //  9   9  \t
		   "\0"     // 10   A  \n
		   "\0"     // 11   B  
		   "\0"     // 12   C  
		   "\0"     // 13   D  \r
		   "\0"     // 14   E  
		   "\0"     // 15   F  
		   "\0"     // 16  10  
		   "\0"     // 17  11  
		   "\0"     // 18  12  
		   "\0"     // 19  13  
		   "\0"     // 20  14  
		   "\0"     // 21  15  
		   "\0"     // 22  16  
		   "\0"     // 23  17  
		   "\0"     // 24  18  
		   "\0"     // 25  19  
		   "\0"     // 26  1A
		   "\0"     // 27  1B  
		   "\0"     // 28  1C  
		   "\0"     // 29  1D  
		   "\0"     // 30  1E  
		   "\0"     // 31  1F  
		   "\1"     // 32  20  blank
		   "\1"     // 33  21  !
		   "\0"     // 34  22  "
		   "\1"     // 35  23  #
		   "\1"     // 36  24  $
		   "\1"     // 37  25  %
		   "\1"     // 38  26  &
		   "\1"     // 39  27  '
		   "\1"     // 40  28  (
		   "\1"     // 41  29  )
		   "\1"     // 42  2A  *
		   "\1"     // 43  2B  +
		   "\1"     // 44  2C  ,
		   "\1"     // 45  2D  -
		   "\1"     // 46  2E  .
		   "\1"     // 47  2F  /
		   "\1"     // 48  30  0
		   "\1"     // 49  31  1
		   "\1"     // 50  32  2
		   "\1"     // 51  33  3
		   "\1"     // 52  34  4
		   "\1"     // 53  35  5
		   "\1"     // 54  36  6
		   "\1"     // 55  37  7
		   "\1"     // 56  38  8
		   "\1"     // 57  39  9
		   "\1"     // 58  3A  :
		   "\1"     // 59  3B  ;
		   "\1"     // 60  3C  <
		   "\1"     // 61  3D  =
		   "\1"     // 62  3E  >
		   "\1"     // 63  3F  ?
		   "\1"     // 64  40  @
		   "\1"     // 65  41  A
		   "\1"     // 66  42  B
		   "\1"     // 67  43  C
		   "\1"     // 68  44  D
		   "\1"     // 69  45  E
		   "\1"     // 70  46  F
		   "\1"     // 71  47  G
		   "\1"     // 72  48  H
		   "\1"     // 73  49  I
		   "\1"     // 74  4A  J
		   "\1"     // 75  4B  K
		   "\1"     // 76  4C  L
		   "\1"     // 77  4D  M
		   "\1"     // 78  4E  N
		   "\1"     // 79  4F  O
		   "\1"     // 80  50  P
		   "\1"     // 81  51  Q
		   "\1"     // 82  52  R
		   "\1"     // 83  53  S
		   "\1"     // 84  54  T
		   "\1"     // 85  55  U
		   "\1"     // 86  56  V
		   "\1"     // 87  57  W
		   "\1"     // 88  58  X
		   "\1"     // 89  59  Y
		   "\1"     // 90  5A  Z
		   "\1"     // 91  5B  [
		   "\1"     // 92  5C  \ .
		   "\1"     // 93  5D  ]
		   "\1"     // 94  5E  ^
		   "\1"     // 95  5F  _
		   "\1"     // 96  60  `
		   "\1"     // 97  61  a
		   "\1"     // 98  62  b
		   "\1"     // 99  63  c
		   "\1"     //100  64  d
		   "\1"     //101  65  e
		   "\1"     //102  66  f
		   "\1"     //103  67  g
		   "\1"     //104  68  h
		   "\1"     //105  69  i
		   "\1"     //106  6A  j
		   "\1"     //107  6B  k
		   "\1"     //108  6C  l
		   "\1"     //109  6D  m
		   "\1"     //110  6E  n
		   "\1"     //111  6F  o
		   "\1"     //112  70  p
		   "\1"     //113  71  q
		   "\1"     //114  72  r
		   "\1"     //115  73  s
		   "\1"     //116  74  t
		   "\1"     //117  75  u
		   "\1"     //118  76  v
		   "\1"     //119  77  w
		   "\1"     //120  78  x
		   "\1"     //121  79  y
		   "\1"     //122  7A  z
		   "\1"     //123  7B  {
		   "\1"     //124  7C  |
		   "\1"     //125  7D  }
		   "\1"     //126  7E  ~
		   "\0"     //127  7F  
		   "\0"     //128  80  �
		   "\0"     //129  81  �
		   "\0"     //130  82  �
		   "\0"     //131  83  �
		   "\0"     //132  84  �
		   "\0"     //133  85  �
		   "\0"     //134  86  �
		   "\0"     //135  87  �
		   "\0"     //136  88  �
		   "\0"     //137  89  �
		   "\0"     //138  8A  �
		   "\0"     //139  8B  �
		   "\0"     //140  8C  �
		   "\0"     //141  8D  �
		   "\0"     //142  8E  �
		   "\0"     //143  8F  �
		   "\0"     //144  90  �
		   "\0"     //145  91  �
		   "\0"     //146  92  �
		   "\0"     //147  93  �
		   "\0"     //148  94  �
		   "\0"     //149  95  �
		   "\0"     //150  96  �
		   "\0"     //151  97  �
		   "\0"     //152  98  �
		   "\0"     //153  99  �
		   "\0"     //154  9A  �
		   "\0"     //155  9B  �
		   "\0"     //156  9C  �
		   "\0"     //157  9D  �
		   "\0"     //158  9E  �
		   "\0"     //159  9F  �
		   "\0"     //160  A0  �
		   "\0"     //161  A1  �
		   "\0"     //162  A2  �
		   "\0"     //163  A3  �
		   "\0"     //164  A4  �
		   "\0"     //165  A5  �
		   "\0"     //166  A6  �
		   "\0"     //167  A7  �
		   "\0"     //168  A8  �
		   "\0"     //169  A9  �
		   "\0"     //170  AA  �
		   "\0"     //171  AB  �
		   "\0"     //172  AC  �
		   "\0"     //173  AD  �
		   "\0"     //174  AE  �
		   "\0"     //175  AF  �
		   "\0"     //176  B0  �
		   "\0"     //177  B1  �
		   "\0"     //178  B2  �
		   "\0"     //179  B3  �
		   "\0"     //180  B4  �
		   "\0"     //181  B5  �
		   "\0"     //182  B6  �
		   "\0"     //183  B7  �
		   "\0"     //184  B8  �
		   "\0"     //185  B9  �
		   "\0"     //186  BA  �
		   "\0"     //187  BB  �
		   "\0"     //188  BC  �
		   "\0"     //189  BD  �
		   "\0"     //190  BE  �
		   "\0"     //191  BF  �
		   "\0"     //192  C0  �
		   "\0"     //193  C1  �
		   "\0"     //194  C2  �
		   "\0"     //195  C3  �
		   "\0"     //196  C4  �
		   "\0"     //197  C5  �
		   "\0"     //198  C6  �
		   "\0"     //199  C7  �
		   "\0"     //200  C8  �
		   "\0"     //201  C9  �
		   "\0"     //202  CA  �
		   "\0"     //203  CB  �
		   "\0"     //204  CC  �
		   "\0"     //205  CD  �
		   "\0"     //206  CE  �
		   "\0"     //207  CF  �
		   "\0"     //208  D0  �
		   "\0"     //209  D1  �
		   "\0"     //210  D2  �
		   "\0"     //211  D3  �
		   "\0"     //212  D4  �
		   "\0"     //213  D5  �
		   "\0"     //214  D6  �
		   "\0"     //215  D7  �
		   "\0"     //216  D8  �
		   "\0"     //217  D9  �
		   "\0"     //218  DA  �
		   "\0"     //219  DB  �
		   "\0"     //220  DC  �
		   "\0"     //221  DD  �
		   "\0"     //222  DE  �
		   "\0"     //223  DF  �
		   "\0"     //224  E0  �
		   "\0"     //225  E1  �
		   "\0"     //226  E2  �
		   "\0"     //227  E3  �
		   "\0"     //228  E4  �
		   "\0"     //229  E5  �
		   "\0"     //230  E6  �
		   "\0"     //231  E7  �
		   "\0"     //232  E8  �
		   "\0"     //233  E9  �
		   "\0"     //234  EA  �
		   "\0"     //235  EB  �
		   "\0"     //236  EC  �
		   "\0"     //237  ED  �
		   "\0"     //238  EE  �
		   "\0"     //239  EF  �
		   "\0"     //240  F0  �
		   "\0"     //241  F1  �
		   "\0"     //242  F2  �
		   "\0"     //243  F3  �
		   "\0"     //244  F4  �
		   "\0"     //245  F5  �
		   "\0"     //246  F6  �
		   "\0"     //247  F7  �
		   "\0"     //248  F8  �
		   "\0"     //249  F9  �
		   "\0"     //250  FA  �
		   "\0"     //251  FB  �
		   "\0"     //252  FC  �
		   "\0"     //253  FD  �
		   "\0"     //254  FE  �
		   "\0";    //255  FF


/********************* end of this file: ascii_7.c **************************/
