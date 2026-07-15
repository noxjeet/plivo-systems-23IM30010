# Protocol Implementation Notes

1. The protocol uses a single-threaded UDP sender and receiver that implements proactive, single-frame redundancy.
2. Each packet contains a 4-byte sequence number, a 1-byte flag indicating the presence of a redundant frame, the 160-byte current frame payload, and an optional 160-byte previous frame payload.
3. Proactive previous-frame redundancy was chosen to recover dropped packets without incurring the round-trip time delay of feedback loops.
4. To stay below the strict 2.00x bandwidth overhead limit, redundancy is selectively omitted on every 20th packet.
5. Retransmissions were intentionally avoided because round-trip times on jittery networks frequently cause retransmitted packets to arrive past their playout deadlines.
6. Duplicate suppression is required at the receiver to prevent forwarding redundant frames or packets duplicated by the network relay.
7. The main limitation of this design is that consecutive packet drops (burst losses) cannot be recovered since redundancy only extends to the immediate previous frame.
8. The recommended grading delay is 98 ms to successfully absorb the maximum 80 ms network transit delay of Profile B while allowing time for redundant frame recovery.
