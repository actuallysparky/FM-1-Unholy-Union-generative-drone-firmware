#!/usr/bin/env python3
"""Regression checks for folded-body accounting and preserved target limits; no target compiler."""
import contextlib, importlib.util, io, os, tempfile, unittest
from pathlib import Path
spec=importlib.util.spec_from_file_location("target_budget",Path(__file__).with_name("target_budget.py"))
b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)
class Accounting(unittest.TestCase):
 def run_gate(self,functions):
  with tempfile.TemporaryDirectory() as tmp:
   dis=Path(tmp)/"code.dis"; budget=Path(tmp)/"budget.txt"
   dis.write_text("\n".join(name+":\n"+"\n".join(f" {i*2:04x}: 00 00  {text}" for i,text in enumerate(body)) for name,body in functions.items()))
   budget.write_text("fm1_alnk0_irq 10\n")
   old=(b.dis,b.budget,b.FUNCS,b.OPTIONAL)
   b.dis,b.budget=str(dis),str(budget); b.FUNCS=["drone_tick","drone_walk","drone_restore","drone_voice","fm1_alnk0_irq"]; b.OPTIONAL={}
   try:
    output=io.StringIO()
    with contextlib.redirect_stdout(output): rc=b.main()
    return rc,output.getvalue()
   finally: b.dis,b.budget,b.FUNCS,b.OPTIONAL=old
 def bodies(self):
  return {"drone_tick":["r0 = r1"],"drone_restore":["r0 = r1"],"drone_voice":["r0 = r1"],"fm1_alnk0_irq":["r0 = r1"]}
 def test_folded_walk_counted_in_parent(self):
  rc,out=self.run_gate(self.bodies()); self.assertEqual(rc,0);self.assertIn("folded into drone_tick",out)
 def test_missing_parent_fails(self):
  f=self.bodies();del f["drone_tick"];rc,out=self.run_gate(f);self.assertEqual(rc,1);self.assertIn("drone_tick not found",out)
 def test_unexpected_separate_walk_requires_review(self):
  f=self.bodies();f["drone_walk"]=["r0 = r1"];rc,out=self.run_gate(f);self.assertEqual(rc,1);self.assertIn("emitted separately",out)
 def test_straight_line_growth_not_free(self):
  f=self.bodies();f["drone_voice"]=["r0 = r1"]*52;rc,out=self.run_gate(f);self.assertEqual(rc,1);self.assertIn("drone_voice insns 52",out)
 def test_inherited_threshold_still_fails(self):
  f=self.bodies();f["fm1_alnk0_irq"]=["r0 = r1"]*11+["goto -22 <fm1_alnk0_irq: 0000 >"]
  rc,out=self.run_gate(f);self.assertEqual(rc,1);self.assertIn("OVER BUDGET",out);self.assertEqual(b.TOL,0.10)
if __name__=="__main__": unittest.main()
