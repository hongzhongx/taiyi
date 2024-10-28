
This is based [Lua 5.3.5](https://github.com/lua/lua/releases/tag/v5.3.5), released on Dec 18 2018.

# Lua Virtual Machine

A Lua virtual machine for blockchain in C/C++ version. Blockchain embed this Lua virtual machine to execute Lua smart contract,  make blockchain as configurable system. Also blockchain can provide syscall for Lua smart contract to interact with each others.

In this Lua virtual machine, io, thread and coroutine libraries are removed, only support basic lua functions.


## Rationale

```
+--------------------------+
|                          |                 +--------------------+
|        Blockchain        | <-------------> |                    |
|                          |                 |                    |
|  +--------------------+  |                 |   Smart Contract   |
|  |                    |  |                 |                    |
|  | Contract Libraries | <----------------> |                    |
|  |                    |  |                 +--------------------+
|  +--------------------+  |
+--------------------------+
```

- Blockchain set fee limit by `lua_setdrops` api to limit contract execution.
- Blockchain call Lua smart contract, push global parameters including `fee limit`, global variable `msg` table as  {`sender`, `address`, `value`, `height`}  and smart contract saved variable `contract`.
- Lua smart contract execute Lua bytecode and stops when bytecode execute finished or contract fee limit run out.
- Smart contract may call blockchain system call libraries when it is necessary.
- Smart contract return one integer to indicate success or failure, when return 0, it is success, otherwise it is failure.
- After smart contract stop, blockchain automatic save smart contract's global variable `contract` if it is modified.

## Fee Cost

For Turing system, it should be stoppable. In blockchain it also should use fee limit to stop smart contract otherwise smart contract will hack down blockchain. In Lua virtual machine, the fee cost include memory cost and instruction cost. Memory cost is used to limit memroy usage, instruction cost is used to limit instruction steps.

### Memory Fee Cost

In Lua virtal machine, smart contract memory used is divided as memory unit. In this virtual machine, the memroy unit is 128 bytes as `CONTRACT_MEM_UNIT_SIZE`. Each memory unit has memory price. The memroy price is 2 CALL drops as `CONTRACT_MEN_UNIT_DROP_COST`.

###  Instruction Fee Cost

Each lua instruction step will consume fee drops, and each lua intruction's fee cost is different. It can be refered at [lplopcodes.c](./lib/lopcodes.c). Virtual machine will stop when smart contract's fee is run out.

## Api

Lua virtual machine provide some extra interfaces to interact with blockchain, including:

### lua_enabledrops

`lua_enabledrops` enable or disable (set to 0) drops support and return pre set. if disable drops, vm actions as normal without fee.

### lua_setdrops

`lua_setdrops` set drops and return 1 if left drops is greater than zero, otherwise return 0.

### lua_getdrops

`lua_getdrops` return currenct drops in lua state of smart contract.
