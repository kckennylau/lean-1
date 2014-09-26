import logic data.prod
open tactic prod

inductive inh (A : Type) : Prop :=
intro : A -> inh A

instance inh.intro

theorem inh_elim {A : Type} {B : Prop} (H1 : inh A) (H2 : A → B) : B
:= inh.rec H2 H1

theorem inh_exists {A : Type} {P : A → Prop} (H : ∃x, P x) : inh A
:= obtain w Hw, from H, inh.intro w

theorem inh_bool [instance] : inh Prop
:= inh.intro true

theorem inh_fun [instance] {A B : Type} (H : inh B) : inh (A → B)
:= inh.rec (λb, inh.intro (λa : A, b)) H

theorem pair_inh [instance] {A : Type} {B : Type} (H1 : inh A) (H2 : inh B) : inh (prod A B)
:= inh_elim H1 (λa, inh_elim H2 (λb, inh.intro (pair a b)))

definition assump := eassumption
tactic_hint assump

theorem tst {A B : Type} (H : inh B) : inh (A → B → B)

theorem T1 {A B C D : Type} {P : C → Prop} (a : A) (H1 : inh B) (H2 : ∃x, P x) : inh ((A → A) × B × (D → C) × Prop) :=
have h1 [visible] : inh A, from inh.intro a,
have h2 [visible] : inh C, from inh_exists H2,
_

(*
print(get_env():find("T1"):value())
*)
