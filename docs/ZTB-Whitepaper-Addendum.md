# ZTB Consensus Refinements for High-Speed Asynchronous Operation

**Version:** 1.0  
**Date:** October 2025  
**Status:** Draft Specification  
**Copyright:** (c) 2025 Cyborg Unicorn Pty Ltd - MIT License

---

## Executive Summary

This addendum details the consensus and security model for the ZOSCII Tamperproof Blockchain (ZTB), designed to achieve sub-second Time-to-Finality (TTF) while maintaining robust security guarantees. The ZTB fundamentally departs from traditional blockchain consensus models by separating block production (asynchronous, high-speed) from consensus reconciliation (periodic, integrity-focused).

**Key Innovations:**
- Policy-driven conflict resolution (Retry vs. Discard)
- Rolling ROM (R-ROM) hash-based synchronization
- Shrinkable tip architecture for non-destructive rollbacks
- Tunable sync frequency for operational flexibility
- Hybrid validator model supporting both permissioned and open networks

---

## I. The Policy-Driven Conflict Resolution Model

### 1.1 Rationale: Retry vs. Discard

To achieve sub-second TTF, the ZTB protocol operates asynchronously, which introduces the risk of conflicting blocks (collisions). The resolution mechanism is made policy-driven to support diverse operational use cases.

#### **Policy A: Rollback + Retry (Lossless Integrity)**

This is the **default policy** for financial ledgers and mission-critical applications.

**Behavior:** When a non-canonical block is rolled back at the shrinkable tip, the protocol automatically:
1. Extracts valid transactions from the rolled-back block
2. Re-queues these transactions into the pending pool
3. Re-encodes and includes them in the next canonical block

**Use Cases:** 
- Financial transactions
- Legal document chains
- Audit trails
- Any application where data loss is unacceptable

**Trade-off:** Slightly higher computational overhead during reconciliation, but guarantees eventual inclusion of all valid transactions.

#### **Policy B: Rollback + Discard (Peak Throughput)**

This policy is selectable for high-frequency data ingestion where absolute peak throughput is prioritized.

**Behavior:** When a non-canonical block is rolled back:
1. Transactions from the rolled-back block are discarded
2. No re-queuing overhead
3. Maximum throughput maintained

**Use Cases:**
- Redundant sensor data (IoT)
- High-frequency market data sampling
- Telemetry streams with acceptable loss
- Applications where throughput > completeness

**Trade-off:** Potential data loss during network splits, but achieves maximum QPS (queries per second).

### 1.2 Conflict Handling at the Shrinkable Tip

The **shrinkable tip** is the architectural innovation that enables both policies. Unlike traditional blockchains where rollbacks require hard forks, ZTB treats tip rollbacks as routine reconciliation.

**Key Properties:**
- Only blocks at the tip (not yet checkpointed) can be rolled back
- Rollback depth is bounded by checkpoint intervals
- Rolling ROM state is preserved during rollback operations
- Shrinking is deterministic and verifiable

---

## II. High-Efficiency Synchronization via Rolling ROM Caching

### 2.1 R-ROM Hashing for Constant-Time Security

The **Cached Rolling ROM (R-ROM)** is a 64KB fixed-size cache that serves as the security context for ZOSCII encoding. Rather than requiring full block validation, the ZTB uses R-ROM hashing to create an instant security state fingerprint.

#### **R-ROM Update Mechanism**

Each `AddBlock` operation involves:
1. **Constant-time 1KB swap:** Remove 1KB from oldest block in cache
2. **Add 1KB from new block:** Append new security context
3. **Hash entire R-ROM:** Generate 32-byte R-ROM Hash

**Computational Complexity:** O(1) for swap + O(64KB) for hash = Constant time per block

**Security Property:** The R-ROM Hash is a cryptographic commitment to the chain's complete security history up to that block.

#### **Why This Works**

Traditional blockchains require recursive Merkle tree computation:
```
Block_N_Hash = Hash(Block_N_Data + Block_N-1_Hash)
```
Verifying Block 1000 requires validating all 1000 previous blocks sequentially.

ZTB's R-ROM approach:
```
R-ROM_Hash_N = Hash(64KB_Rolling_Window_at_Block_N)
```
The R-ROM Hash **is** the security state. Matching R-ROM hashes = matching verified history.

### 2.2 Asynchronous Synchronization Protocol

#### **Sync Check Efficiency**

Instead of transferring full blocks, nodes exchange only their latest R-ROM Hash (32 bytes).

**Best Case (Synchronized):**
```
Node A: R-ROM Hash = 0x4A8F3C2B...
Node B: R-ROM Hash = 0x4A8F3C2B...
Result: Nodes are in sync. No further action needed.
Cost: 32 bytes exchanged
```

**Divergence Case:**
```
Node A: R-ROM Hash = 0x4A8F3C2B...
Node B: R-ROM Hash = 0x7D1E9F8A...
Result: Nodes have diverged. Initiate targeted resync.
```

#### **Mitigation of Sync Delay**

A node that has been offline for an extended period (e.g., >1 hour ≈ 5,000 blocks at 1 block/sec) can:

1. **Request R-ROM Hash List:** A compact list of recent R-ROM hashes
   - Example: Last 5,000 hashes = 5,000 × 32 bytes = 160KB
2. **Find Last Common Hash:** Binary search to identify divergence point
3. **Calculate Divergence Depth:** Number of blocks built after common ancestor
4. **Targeted Resync:** Download only the canonical blocks after divergence point

**Efficiency Gain:** 
- Traditional: Download and validate 5,000 blocks sequentially (~hours)
- ZTB: Compare 160KB hash list, download only divergent blocks (~seconds)

---

## III. Dynamic Consensus and Reconciliation Protocol

The ZTB does not rely on a single, continuous consensus event. Instead, it uses a rapid, multi-staged process to convert asynchronous "tentative" blocks into a single, canonical, immutable history.

### 3.1 Consensus Event (Master Trunk Selection)

When the network initiates a consensus round (triggered by timer or performance threshold):

#### **Metric 1: Longest Tree Preference**

The simplest metric selects the trunk with the most blocks as the initial candidate.

**Rationale:** Rewards network liveness and productivity.

#### **Metric 2: R-ROM Hash Consensus (Weighted Majority)**

The more robust metric uses R-ROM Hash Lists to identify the trunk with the deepest verified security state.

**Algorithm:**
1. All nodes broadcast their last N R-ROM hashes (e.g., N=100)
2. For each competing trunk, count how many consecutive R-ROM hashes match the majority
3. The trunk with the longest consecutive hash agreement wins

**Rationale:** Ensures the selected trunk has the most widely verified history.

#### **Tiebreaker Cascade**

If multiple trunks are still tied after applying both metrics:

1. **Earliest Divergence Timestamp:** Trunk that diverged first (closest to common ancestor)
2. **Lexicographically Smallest Trunk ID:** Deterministic, reproducible across all nodes
3. **Lowest Hash Value:** Final deterministic tiebreaker using the hash of the latest block

**Example:**
```
Trunk A: 1000 blocks, 80% hash consensus, diverged at 12:00:00, ID="alpha"
Trunk B: 1000 blocks, 80% hash consensus, diverged at 12:00:01, ID="beta"
Result: Trunk A wins (earlier divergence)
```

### 3.2 Reconciliation of Non-Canonical Trunks

Once the Master Trunk is identified, all competing trunks must reconcile.

#### **Step A: Find the Common Ancestor**

Using the R-ROM Hash List, binary search to find the last common block shared between the non-canonical trunk and the Master Trunk.

**Efficiency:** O(log N) comparisons to find divergence point in N blocks.

#### **Step B: Shrink the Tip (Rollback)**

The non-canonical node removes all blocks back to the common ancestor.

**Shrink Operation:**
1. Delete block files from disk
2. Reverse R-ROM swaps (remove added 1KB, restore removed 1KB)
3. Update chain state metadata

**Verification:** After shrinking, the node's R-ROM Hash must match the common ancestor's hash.

#### **Step C: Data Extraction**

Transactions from removed blocks are extracted based on the configured policy:

**If `Conflict_Policy: Discard`:**
- Extracted data is deleted
- No re-queuing overhead
- Maximum performance mode

**If `Conflict_Policy: Retry`:**
- Valid transactions extracted
- Added to pending transaction pool with retry metadata
- Marked for re-inclusion in next block

### 3.3 Re-Queuing and Final Commitment

This is the crucial step that ensures lossless data integrity for the Retry policy.

#### **Sync to Master Peak**

The "shrunk" node receives blocks from the Master Trunk:
1. Downloads blocks from common ancestor to Master Trunk's latest
2. Applies R-ROM swaps in order
3. Verifies R-ROM Hash matches Master Trunk

**Verification:** Final R-ROM Hash must match the Master Trunk's current state.

#### **Re-Queue (Retry Phase)**

Extracted transactions from non-canonical blocks are:
1. Re-validated (check for conflicts with canonical blocks)
2. Re-encoded with new random ZOSCII addresses
3. Added to the transaction pool
4. Included in the next block proposed by this node

**Transaction Lifecycle:**
```
PENDING → TENTATIVE → ROLLED_BACK → RETRYING → CANONICAL
                                          ↓
                                       EXPIRED (if TTL exceeded)
```

---

## IV. Dynamic Synchronization Frequency

To maintain sub-second TTF, global synchronization checks **cannot** be mandatory for every block addition. The frequency must be an operator-tuned parameter.

### 4.1 Default Asynchronous Operation

**Normal Mode:**
- Nodes operate asynchronously by default
- Each node trusts its locally verified R-ROM Hash
- Block production focuses on 1-second target time
- No global broadcast required for each block

### 4.2 Tunable Sync Check Frequency

The explicit broadcast sync check is configurable:

#### **Time-Based (Periodic) Sync**

A simple time interval triggers consensus rounds.

**Example Configurations:**
- **Aggressive:** Every 1 minute (high consistency, moderate overhead)
- **Balanced:** Every 5 minutes (default for most use cases)
- **Relaxed:** Every 15-60 minutes (maximum throughput, eventual consistency)

**Use Case:** Predictable, low-overhead synchronization checks to detect long-term network drift.

#### **Performance-Based (Trigger) Sync**

Consensus round is triggered when the network hits a performance threshold:

**Trigger Conditions:**
- Collision rate exceeds threshold (e.g., >10% of blocks are conflicting)
- Sudden spike in conflicting block proposals
- Node reports high divergence depth

**Use Case:** Addresses peak load/congestion. A sudden high collision rate signals that a sync check is needed to prevent divergence before it gets too deep.

#### **Hybrid Approach (Recommended)**

Combine both strategies:
```
sync_check_triggers = {
  "time_based": "300s",  // Every 5 minutes minimum
  "collision_rate": "0.10",  // If >10% blocks conflicting
  "max_divergence": "500"  // If any node >500 blocks diverged
}
```

### 4.3 Divergence Depth Monitoring

The primary operational metric is **Divergence Depth**: the number of blocks that a minority trunk has built past the last confirmed common R-ROM hash.

**Health Indicators:**
- **Low Divergence (<100 blocks):** Network operating efficiently
- **Moderate Divergence (100-500 blocks):** May need sync check soon
- **High Divergence (>500 blocks):** Emergency sync check recommended
- **Critical Divergence (>checkpoint interval):** Manual intervention required

---

## V. Validator Model and Network Security

### 5.1 Supported Validator Models

ZTB supports multiple validator models depending on deployment requirements:

#### **Model A: Permissioned Network (Default for Enterprise)**

**Configuration:**
```yaml
validator_model:
  type: "permissioned"
  validators:
    - id: "validator01"
      pubkey: "0x4A8F3C2B..."
      weight: 1
    - id: "validator02"
      pubkey: "0x7D1E9F8A..."
      weight: 1
  consensus_threshold: 0.67  # Requires 2/3+ agreement
  byzantine_fault_tolerance: true
```

**Properties:**
- Fixed, known validator set
- Each validator has equal or weighted vote
- Byzantine Fault Tolerant (BFT): tolerates up to 1/3 malicious validators
- Sybil attack prevention: identity-based

**Use Cases:**
- Enterprise consortiums
- Supply chain tracking
- Inter-organizational ledgers
- Regulatory compliance scenarios

#### **Model B: Proof-of-Stake (PoS) Weighted**

**Configuration:**
```yaml
validator_model:
  type: "proof_of_stake"
  minimum_stake: 1000  # Minimum tokens to become validator
  stake_token: "ZTB"
  vote_weight: "stake_proportional"  # Vote weight = stake amount
  slashing_enabled: true
  slashing_penalty: 0.1  # 10% stake slashed for malicious behavior
```

**Properties:**
- Open participation (anyone can stake)
- Economic incentive alignment
- Vote weight proportional to staked amount
- Slashing penalties deter malicious behavior

**Use Cases:**
- Public networks with token economy
- Decentralized applications
- Community-governed chains

#### **Model C: Hybrid Reputation-Based**

**Configuration:**
```yaml
validator_model:
  type: "hybrid_reputation"
  reputation_factors:
    - uptime: 0.3
    - correct_proposals: 0.4
    - stake: 0.2
    - age: 0.1
  minimum_reputation: 0.5
  new_node_weight: 0.1  # New nodes start with low weight
  reputation_decay: 0.01  # Per epoch
```

**Properties:**
- Reputation earned over time
- Multi-factor trust calculation
- New nodes can participate but with limited influence
- Gradual trust accumulation

**Use Cases:**
- Semi-open networks
- Balancing security with accessibility
- Long-term node incentivization

### 5.2 Sybil Attack Prevention

Each validator model includes specific Sybil resistance:

**Permissioned:** Identity-based whitelist
**PoS:** Economic cost (must stake tokens)
**Hybrid:** Time-based reputation accumulation

**Additional Mitigation:**
- IP diversity requirements (validators must be geographically distributed)
- Rate limiting on vote submission
- Minimum node uptime requirements

### 5.3 Bootstrap and Trust Model

When a new node joins the network:

#### **Option 1: Trusted Bootstrap Nodes**

Genesis configuration includes a list of trusted seed nodes:
```yaml
bootstrap:
  seed_nodes:
    - "node1.ztb.network:8332"
    - "node2.ztb.network:8332"
    - "node3.ztb.network:8332"
  require_minimum: 2  # Must connect to at least 2 seeds
```

**Process:**
1. New node connects to seed nodes
2. Requests R-ROM Hash List from multiple seeds
3. Verifies consensus among seeds (>50% agreement)
4. Downloads blocks from trusted sources

#### **Option 2: Checkpoint Signatures**

Validators sign R-ROM hashes at checkpoint intervals:
```yaml
checkpoint:
  interval: 1000  # Every 1000 blocks
  required_signatures: 5  # Need 5 validator signatures
  validator_pubkeys: [...]  # List of validator public keys
```

**Process:**
1. New node downloads checkpoint signatures
2. Verifies signatures against known validator keys
3. Trusts the signed R-ROM Hash
4. Downloads blocks from checkpoint forward

#### **Option 3: Social Consensus (Out-of-Band)**

For critical deployments, verify the latest R-ROM Hash through external channels:
- Official website or block explorer
- Social media verification
- Email confirmation from network operators

Similar to Ethereum's "weak subjectivity" approach.

---

## VI. Attack Mitigation and Security Analysis

### 6.1 Re-Queue Attack Vector

**Attack Scenario:**
1. Attacker creates block with spam transactions
2. Block gets rolled back (non-canonical)
3. Spam transactions re-queued
4. Attacker forces another collision
5. Repeat → DoS via re-queue flooding

**Mitigations:**

#### **A. Transaction Time-to-Live (TTL)**
```yaml
transaction_ttl: 3600  # seconds (1 hour)
```
Transactions expire after TTL, preventing infinite retry.

#### **B. Re-Queue Attempt Limits**
```yaml
max_requeue_attempts: 5
```
After 5 rollback attempts, transaction is permanently discarded.

#### **C. Priority Queue by Age**
```yaml
requeue_priority: "age"  # Older transactions prioritized
```
Prevents new spam from flooding the queue.

**Combined Protection:**
```
Transaction created at T=0
Rolled back at T=300 (attempt 1)
Rolled back at T=600 (attempt 2)
...
Rolled back at T=1500 (attempt 5)
Result: Transaction discarded (max attempts exceeded)
```

### 6.2 Network Split Recovery

**Catastrophic Split Scenario:**
```
Network splits 50/50 for 24 hours
Each side builds 86,400 blocks independently
Divergence depth: 86,400 blocks
```

**Recovery Mechanism:**

#### **Checkpoint Finality System**
```yaml
checkpoint:
  interval: 1000  # blocks
  finality: true  # Checkpoints are irreversible
```

**Rules:**
- Divergence **within** a checkpoint interval: automatic reconciliation
- Divergence **beyond** a checkpoint: manual intervention required

**Recovery Process:**
1. Network operator identifies the split
2. Determines which side to preserve (usually longer chain)
3. Notifies nodes on non-canonical side
4. Provides recovery script to migrate users
5. Non-canonical side can optionally continue as separate network (hard fork)

#### **Emergency Consensus Mode**
```yaml
emergency_mode:
  divergence_threshold: 1000  # blocks
  consensus_requirement: 0.90  # Require 90% agreement
  block_rate_reduction: 0.5  # Halve block rate
```

When divergence exceeds threshold:
1. Enter emergency mode
2. Increase consensus requirement from 67% to 90%
3. Reduce block rate to allow more time for consensus
4. Continue until convergence achieved

### 6.3 Genesis ROM Security

**Critical Dependency:** The Genesis ROM is the root of trust. Loss = unrecoverable chain.

**Protection Measures:**
1. **Redundant Storage:** Store Genesis ROM in multiple locations
2. **Checksum Verification:** Regularly verify Genesis ROM integrity
3. **Access Control:** Restrict Genesis ROM access to authorized operators
4. **Backup Strategy:** Include Genesis ROM in all backups
5. **Public Archive:** For public chains, publish Genesis ROM hash for verification

### 6.4 R-ROM Hash Collision Risk

**Theoretical Risk:** SHA-256 hash collisions

**Analysis:**
- 32-byte hash = 2^256 possible values
- Birthday paradox: collisions likely after 2^128 hashes
- At 1 block/sec: 2^128 seconds ≈ 10^31 years

**Conclusion:** Collision risk is negligible for practical purposes.

**Additional Protection:** If quantum computing advances threaten SHA-256, the R-ROM hashing algorithm can be upgraded without breaking existing blocks (only affects new blocks going forward).

---

## VII. Finality Guarantees

### 7.1 Probabilistic Finality

A transaction achieves increasing finality confidence with each consensus round that confirms it:

**Confirmation Levels:**
- **0 confirmations:** Block added, tentative state
- **1 confirmation:** Survived first sync check (≈99% confidence)
- **3 confirmations:** Survived three sync checks (≈99.9% confidence)
- **6 confirmations:** High confidence (≈99.99% confidence)

**Recommendation:** 
- Low-value transactions: 1 confirmation acceptable
- Medium-value: 3 confirmations
- High-value: 6+ confirmations or wait for checkpoint

### 7.2 Absolute Finality (Checkpoint System)

Checkpoints provide irreversible finality:

```yaml
checkpoint:
  interval: 1000  # Every 1000 blocks
  validator_signatures_required: 5
  automatic: true
```

**Checkpoint Properties:**
- Cannot be rolled back
- Signed by multiple validators
- Serves as new "genesis" for pruning purposes
- Enables safe archival of old blocks

**Finality Timeline:**
```
Block 0 (Genesis) → [1000 blocks] → Checkpoint 1 (finalized)
                                   → [1000 blocks] → Checkpoint 2 (finalized)
```

Blocks before the last checkpoint are **absolutely final** and cannot be reverted.

---

## VIII. Operational Configuration

### 8.1 Recommended Configuration Profiles

#### **Profile: Financial Ledger**
```yaml
network_profile: "financial_ledger"
conflict_policy: "retry"  # Lossless integrity
transaction_ttl: 86400  # 24 hours
max_requeue_attempts: 10
sync_interval: 300  # 5 minutes
checkpoint_interval: 1000
validator_model: "permissioned"
consensus_threshold: 0.67
```

#### **Profile: IoT Telemetry**
```yaml
network_profile: "iot_telemetry"
conflict_policy: "discard"  # Peak throughput
transaction_ttl: 300  # 5 minutes
max_requeue_attempts: 0  # No retries
sync_interval: 900  # 15 minutes
checkpoint_interval: 5000
validator_model: "hybrid_reputation"
```

#### **Profile: Public Blockchain**
```yaml
network_profile: "public_blockchain"
conflict_policy: "retry"
transaction_ttl: 3600  # 1 hour
max_requeue_attempts: 5
sync_interval: 300  # 5 minutes
checkpoint_interval: 2016  # ~2 weeks at 1 block/sec
validator_model: "proof_of_stake"
minimum_stake: 1000
consensus_threshold: 0.67
```

### 8.2 Monitoring and Alerting

#### **Critical Metrics**

**Real-Time:**
- Current divergence depth (per node)
- Collision rate (blocks/sec rejected)
- Re-queue depth (pending retries)
- R-ROM hash agreement percentage

**Historical:**
- Longest divergence event
- Average time to convergence
- Re-queue success rate
- Network split frequency

**Alert Thresholds:**
```yaml
alerts:
  divergence_depth_warning: 200
  divergence_depth_critical: 500
  collision_rate_warning: 0.15  # 15%
  collision_rate_critical: 0.30  # 30%
  sync_check_failures: 3  # consecutive
  requeue_depth_warning: 5000
  requeue_depth_critical: 10000
```

---

## IX. Implementation Roadmap

### Phase 1: Core Consensus (Weeks 1-2)
- [ ] Implement R-ROM hash calculation
- [ ] Build shrinkable tip mechanism
- [ ] Develop sync check protocol
- [ ] Create tiebreaker cascade logic
- [ ] Add divergence depth monitoring

### Phase 2: Validator Models (Weeks 3-4)
- [ ] Implement permissioned validator model
- [ ] Add PoS weighting system
- [ ] Build reputation calculation engine
- [ ] Develop Sybil resistance measures
- [ ] Create bootstrap protocol

### Phase 3: Attack Mitigation (Week 5)
- [ ] Implement transaction TTL
- [ ] Add re-queue attempt limits
- [ ] Build priority queue system
- [ ] Develop emergency consensus mode
- [ ] Create checkpoint finality system

### Phase 4: Testing & Validation (Weeks 6-8)
- [ ] Simulate network splits
- [ ] Test catastrophic divergence
- [ ] Benchmark sync performance
- [ ] Stress test re-queue system
- [ ] Validate Byzantine fault tolerance

### Phase 5: Production Hardening (Weeks 9-10)
- [ ] Build monitoring dashboard
- [ ] Create operator runbook
- [ ] Document failure modes
- [ ] Implement logging and metrics
- [ ] Security audit

---

## X. Conclusion

The ZTB consensus model achieves sub-second finality while maintaining robust security through:

1. **Separation of Concerns:** Block production (fast, async) vs. consensus reconciliation (periodic, thorough)
2. **R-ROM Hash Innovation:** Constant-time security fingerprinting enables instant sync verification
3. **Policy Flexibility:** Retry vs. Discard accommodates diverse use cases
4. **Shrinkable Tip:** Non-destructive rollbacks eliminate hard fork requirements
5. **Comprehensive Security:** Multiple validator models, attack mitigation, and finality guarantees

This design represents a fundamental rethinking of blockchain consensus, optimized for the realities of high-speed asynchronous operation while preserving the security guarantees that make blockchains trustworthy.

---

## Appendix A: Configuration Reference

```yaml
# Complete ZTB Configuration Template
network:
  name: "ZTB Network"
  genesis_rom: "/path/to/genesis.rom"
  
consensus:
  # Conflict resolution policy
  conflict_policy: "retry"  # or "discard"
  
  # Synchronization
  sync_interval: 300  # seconds
  sync_triggers:
    collision_rate: 0.10  # 10%
    max_divergence: 500  # blocks
  
  # Tiebreaker rules (applied in order)
  tiebreakers:
    - "longest_tree"
    - "hash_consensus"
    - "earliest_divergence"
    - "lexicographic_id"
    - "lowest_hash"

validator:
  model: "permissioned"  # or "proof_of_stake", "hybrid_reputation"
  
  # Permissioned model
  validators:
    - id: "validator01"
      pubkey: "0x..."
      weight: 1
  
  consensus_threshold: 0.67  # 2/3+ majority
  byzantine_fault_tolerance: true

transactions:
  ttl: 3600  # seconds
  max_requeue_attempts: 5
  requeue_priority: "age"  # or "fee", "random"

checkpoint:
  interval: 1000  # blocks
  finality: true
  required_signatures: 5
  automatic: true

monitoring:
  metrics_port: 9090
  dashboard_enabled: true
  alert_webhook: "https://alerts.example.com/ztb"
  
  thresholds:
    divergence_warning: 200
    divergence_critical: 500
    collision_rate_warning: 0.15
    collision_rate_critical: 0.30

bootstrap:
  seed_nodes:
    - "node1.ztb.network:8332"
    - "node2.ztb.network:8332"
  require_minimum: 2
  checkpoint_verification: true
```

---

## Appendix B: Glossary

**R-ROM (Rolling ROM):** 64KB cache of security context sampled from previous blocks  
**R-ROM Hash:** 32-byte cryptographic fingerprint of the R-ROM state  
**Shrinkable Tip:** The most recent blocks that can be rolled back during reconciliation  
**Divergence Depth:** Number of blocks built past the last common R-ROM hash  
**Master Trunk:** The canonical chain selected during consensus rounds  
**Checkpoint:** An irreversible finality marker signed by validators  
**TTL (Time-to-Live):** Maximum age a transaction can reach before expiring  
**Collision:** Multiple competing blocks proposed at the same height  
**Tentative Block:** A block that has been added but not yet confirmed by consensus  
**Canonical Block:** A block confirmed as part of the master trunk

---

**End of Addendum**