# VottunBridge CLI Testing Guide

g++ -o qubic-cli main.cpp connection.cpp keyUtils.cpp nodeUtils.cpp walletUtils.cpp vottunBridge.cpp assetUtils.cpp SCUtils.cpp fileUpload.cpp msvault.cpp qvault.cpp qx.cpp qearn.cpp proposal.cpp qutil.cpp quottery.cpp testUtils.cpp -lcurl -lpthread -std=c++17

## Network Configuration
- **Node IP**: 194.247.186.29
- **Node Port**: 31841
- **Contract Index**: 18

## Common Variables
```bash
# Replace these values with your actual data
SEED="your_55_character_seed_here"
IDENTITY="YOUR_QUBIC_IDENTITY_HERE"
ETH_ADDRESS="ethereum_address_in_64_byte_hex_format"
TICK_OFFSET=10  # Number of ticks in the future to execute the transaction
```

---

## 1. QUERY FUNCTIONS (No seed required)

### 1.1 Get contract information
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -vottungetcontractinfo
```
**Description**: Retrieves all contract information including:
- Main admin
- Manager list (16 slots)
- Next Order ID
- Locked and received tokens
- Fees earned
- Multisig configuration
- First 16 orders

---

### 1.2 Query a specific order
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -vottungetorder <ORDER_ID>
```
**Example**:
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -vottungetorder 1
```
**Description**: Retrieves details of a specific order by its ID.

---

### 1.3 Get total received tokens
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -vottungettotalreceivedtoken <AMOUNT>
```
**Example**:
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -vottungettotalreceivedtoken 1000000
```

---

### 1.4 Get total locked tokens
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -vottungettotallockedtokens
```

---

### 1.5 Find order by details
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -vottungetorderbydetails <ETH_ADDRESS> <AMOUNT> <STATUS>
```
**Example**:
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -vottungetorderbydetails "QUBIC_IDENTITY" 1000000 1
```

---

### 1.6 Get available fees
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -vottungetavailablefees
```
**Description**: Shows available fees, total earned, and total distributed.

---

### 1.7 Get multisig proposal
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -vottungetproposal <PROPOSAL_ID>
```
**Example**:
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -vottungetproposal 1
```

---

## 2. USER OPERATIONS (Require seed)

### 2.1 Create an order (Bridge)
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -seed <SEED> -scheduledTickOffset <OFFSET> -vottuncreateorder <QUBIC_DESTINATION> <ETH_ADDRESS> <AMOUNT> <FROM_QUBIC_TO_ETH>
```

**Example - From Qubic to Ethereum**:
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -seed "your_seed_here" -scheduledTickOffset 10 -vottuncreateorder "RECIPIENT_QUBIC" "eth_address_hex_64bytes" 10000000 1
```

**Example - From Ethereum to Qubic**:
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -seed "your_seed_here" -scheduledTickOffset 10 -vottuncreateorder "RECIPIENT_QUBIC" "eth_address_hex_64bytes" 10000000 0
```

**Important notes**:
- Fee is 0.5% (calculated automatically)
- `<AMOUNT>`: Amount in Qubic base units
- `<FROM_QUBIC_TO_ETH>`: 1 = Qubic→ETH, 0 = ETH→Qubic
- Transaction includes the total calculated fee

---

### 2.2 Add liquidity
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -seed <SEED> -scheduledTickOffset <OFFSET> -vottunaddliquidity <AMOUNT>
```

**Example**:
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -seed "your_seed_here" -scheduledTickOffset 10 -vottunaddliquidity 50000000
```

---

### 2.3 Transfer to contract
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -seed <SEED> -scheduledTickOffset <OFFSET> -vottuntransfertocontract <AMOUNT> <ORDER_ID>
```
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -seed xqykhnhpgyoylpojfkxrebndtbsdmlbitvujvfmuhljoynhkdbmquxe -scheduledTickOffset 35462000 -vottuntransfertocontract 10000 3

**Example**:
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -seed "your_seed_here" -scheduledTickOffset 10 -vottuntransfertocontract 10000000 1
```

**Description**: Any user can send tokens to the contract for a specific order (ETH→Qubic direction).

---

## 3. MANAGER OPERATIONS (Require manager permissions)

### 3.1 Complete an order
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -seed <MANAGER_SEED> -scheduledTickOffset <OFFSET> -vottuncompleteorder <ORDER_ID>
```

**Example**:
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -seed "manager_seed" -scheduledTickOffset 10 -vottuncompleteorder 1
```

---

### 3.2 Refund an order
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -seed <MANAGER_SEED> -scheduledTickOffset <OFFSET> -vottunrefundorder <ORDER_ID>
```

**Example**:
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -seed "manager_seed" -scheduledTickOffset 10 -vottunrefundorder 1
```

---

## 4. ADMIN OPERATIONS (Require admin permissions)

### 4.1 Add manager
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -seed <ADMIN_SEED> -scheduledTickOffset <OFFSET> -vottunaddmanager <MANAGER_IDENTITY>
```

**Example**:
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -seed "admin_seed" -scheduledTickOffset 10 -vottunaddmanager "MANAGER_IDENTITY"
```

---

### 4.2 Remove manager
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -seed <ADMIN_SEED> -scheduledTickOffset <OFFSET> -vottunremovemanager <MANAGER_IDENTITY>
```

**Example**:
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -seed "admin_seed" -scheduledTickOffset 10 -vottunremovemanager "MANAGER_IDENTITY"
```

---

### 4.3 Withdraw fees
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -seed <ADMIN_SEED> -scheduledTickOffset <OFFSET> -vottunwithdrawfees <AMOUNT>
```

**Example**:
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -seed "admin_seed" -scheduledTickOffset 10 -vottunwithdrawfees 1000000
```

---

## 5. MULTISIG OPERATIONS (Proposal system)

### 5.1 Create proposal
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -seed <MULTISIG_ADMIN_SEED> -scheduledTickOffset <OFFSET> -vottuncreateproposal <PROPOSAL_TYPE> <TARGET_ADDRESS> <OLD_ADDRESS> <AMOUNT>
```

**Proposal types**:
- `1`: Add multisig admin
- `2`: Remove multisig admin
- `3`: Change approval requirement
- `4`: Withdraw fees
- (Other types according to contract)

**Example - Add multisig admin**:
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -seed "multisig_admin_seed" -scheduledTickOffset 10 -vottuncreateproposal 1 "NEW_ADMIN_IDENTITY" "" 0
```

**Example - Withdraw fees**:
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -seed "multisig_admin_seed" -scheduledTickOffset 10 -vottuncreateproposal 4 "" "" 1000000
```

---

### 5.2 Approve proposal
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -seed <MULTISIG_ADMIN_SEED> -scheduledTickOffset <OFFSET> -vottunapproveproposal <PROPOSAL_ID>
```

**Example**:
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -seed "multisig_admin_seed" -scheduledTickOffset 10 -vottunapproveproposal 1
```

---

## 6. RECOMMENDED TESTING FLOW

### Step 1: Verify initial state
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -vottungetcontractinfo
```

### Step 2: Create a test order
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -seed "your_seed" -scheduledTickOffset 10 -vottuncreateorder "DEST_IDENTITY" "eth_address_hex" 1000000 1
```

### Step 3: Verify the created order
```bash
# Get the ORDER_ID from the previous step
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -vottungetorder <ORDER_ID>
```

### Step 4: As manager, complete the order
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -seed "manager_seed" -scheduledTickOffset 10 -vottuncompleteorder <ORDER_ID>
```

### Step 5: Verify accumulated fees
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -vottungetavailablefees
```

### Step 6: Verify final state
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -vottungetcontractinfo
```

---

## 7. TRANSACTION VERIFICATION

After each transaction, the CLI will display a command to verify the status:
```bash
./qubic-cli -nodeip 194.247.186.29 -nodeport 31841 -checktxontick <TICK_NUMBER> <TX_HASH>
```

---

## 8. IMPORTANT NOTES

1. **Seeds**: Never share your seeds. Use them only in secure environments.
2. **Tick Offset**: Adjust according to network congestion (10-20 ticks is recommended).
3. **Fees**: The system automatically calculates 0.5% fee in `createOrder`.
4. **Permissions**: Verify that your identity has the necessary permissions before executing administrative operations.
5. **ETH Address Format**: Must be an Ethereum address in 64-byte hexadecimal format.
6. **Multisig**: Critical operations require multiple approvals according to contract configuration.

---

## 9. STATUS CODES

### Order Status
- `0`: Success
- `1`: Pending
- `2`: Completed
- `3`: Refunded
- (Others according to contract)

### Proposal Status
- `0`: Active
- `1`: Executed
- (Others according to contract)

---

## 10. TROUBLESHOOTING

### Error: "Failed to receive data"
- Check connectivity with the node
- Ensure the node is synchronized

### Error: Transaction rejected
- Verify you have sufficient balance
- Confirm you have the necessary permissions
- Increase the `scheduledTickOffset`

### Error: Invalid identity
- Verify the identity format (60 uppercase characters)
- Ensure you're using the correct address format

---

## Contact and Support
To report issues or request help, consult the project documentation or contact the development team.

Admin seeds to test:

"PTISGHDLNMNEBASUPNOACIMCJTNADIPRCKDUZLJPACIIEITJKQGYFYDHIIVF",xqykhnhpgyoylpojfkxrebndtbsdmlbitvujvfmuhljoynhkdbmquxe
"EQMBBVYGZOFUIHEXFOXKTFTANEKBXLBXHAYDFFMREEMRQEVADYMMEWACTODD",xpsxzzfqvaohzzwlbofvqkqeemzhnrscpeeokoumekfodtgzmwghtqm
"HYJXEZSECWSKODJALRCKSLKVYUEBMAHDODYZUJIIYDPAGFKLMOTHTJXEBEWM",ukzbkszgzpipmxrrqcxcppumxoxzerrvbjgthinzodrlyblkedutmsy