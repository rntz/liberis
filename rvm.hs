module RVM where

-- This isn't the actual vm.
type Reg = Int
type Upvar = Int
type Dest = Reg

-- FIXME: how do upvars get introduced?
-- call to primitive funcs?
-- instruction that looks up in context?

data Const = CInt Int
           | CString String
             deriving Show

data Arg = AReg Reg
         | AUpvar Upvar
         | AConst Const
           deriving Show

data FuncArg = FUpvar Upvar
             | FReg Reg
               deriving Show

data Cmp = Is Ordering | Isnt Ordering
           deriving Show

data Inst = Move Dest Arg
          -- xxx hack.
          | Lookup Dest String
          -- Call func arg0 nargs nrets
          -- is nrets necessary/useful?
          | Call FuncArg Reg Int Int
          | Jump Int            -- relative offset
            -- Cond c x y n
            -- if (c x y), go to next instr; otherwise, jump n
          | CondJump Cmp Arg Arg Int
          | Close Dest Func [Arg]
          | Return Reg Int
            deriving Show

data Func = Func { funcMinArgs :: Int
                 , funcMaxArgs :: Maybe Int
                 , funcNumRegs :: Int -- >= funcNumArgs
