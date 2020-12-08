<h1 class="contract">init</h1>

---
spec_version: "0.1.0"
title: init to start election
summary: 'Init Election'
icon: http://127.0.0.1/ricardian_assets/mgp.contracts/icons/
---

Kick start the election by invoking init action

<h1 class="contract">config</h1>

---
spec_version: "0.1.0"
title: config global params
summary: 'Configure Global Params'
icon: http://127.0.0.1/ricardian_assets/mgp.contracts/icons/
---

Making updates to global params

<h1 class="contract">chvote</h1>

---
spec_version: "0.1.0"
title: change vote
summary: 'Change one's vote'
icon: http://127.0.0.1/ricardian_assets/mgp.contracts/icons/
---

Change one's vote

<h1 class="contract">unvote</h1>

---
spec_version: "0.1.0"
title: Unvote
summary: 'Withdraw tokens to unvote'
icon: http://127.0.0.1/ricardian_assets/mgp.contracts/icons/
---

Withdraw one's tokens to unvote but have to wait for min. 3 days before unvote.

<h1 class="contract">execute</h1>

---
spec_version: "0.1.0"
title: Execute
summary: 'Execute election'
icon: http://127.0.0.1/ricardian_assets/mgp.contracts/icons/
---

Execute election repeatedly until completion. Each step invovles 3 sub-processes:
1) scan for votes in the one before last eletion round;
2) scan for unvotes in the last election round;
3) distribute rewards accordingly (by recording instead of transferring)


<h1 class="contract">delist</h1>

---
spec_version: "0.1.0"
title: Delist
summary: 'Delist a candidate'
icon: http://127.0.0.1/ricardian_assets/mgp.contracts/icons/
---

A candidate tries to delist itself from the candidates list

<h1 class="contract">claimrewards</h1>

---
spec_version: "0.1.0"
title: claim rewards
summary: 'Claim one's rewards due'
icon: http://127.0.0.1/ricardian_assets/mgp.contracts/icons/
---

Either a candidate or a voter tries to claim rewards due in the contract
