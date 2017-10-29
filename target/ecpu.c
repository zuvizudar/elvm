#include <ir/ir.h>
#include <target/util.h>
#include<string.h>
int ecpu_label[31];
int ecpu_cnt;
long long d2b(int decimal){
	long long  binary=0;
	long long  base=1;
	while(decimal>0){
		binary+=(decimal%2)*base;
		decimal/=2;
		base*=10;
	}

	return binary;
}
static const char* reg_names_ecpu[]={
	"000","001","010","011","100","101"
};
int label_ok=0,label_cnt=0;
const char* ecpu_cmp_str(Inst* inst) {
  int op = normalize_cond(inst->op, 0);
  const char* op_str;
  switch (op) {
    case JEQ:
      op_str = "01110"; break;
    case JNE:
      op_str = "01111"; break;
    case JLT:
      op_str = "10000"; break;
    case JGT:
      op_str = "10001"; break;
    case JLE:
      op_str = "10010"; break;
    case JGE:
      op_str = "10011"; break;
    case JMP:
      op_str = "10100"; break;
    default:
      error("oops");
  }
  return op_str;
}
const char* ecpu_value_str(Value* v) {
  if (v->type == REG) {
    return reg_names_ecpu[v->reg];
  } else if (v->type == IMM) {
    return format("%d",d2b(v->imm));
  } else {
    error("invalid value");
  }
}

static void ecpu_init_state(void) {
}


static void ecpu_emit_func_prologue(int func_id) {
	func_id++; //dummy
}

static void ecpu_emit_func_epilogue(void) {
}

static void ecpu_emit_pc_change(int pc) {
	if(label_ok==0){
		ecpu_cnt++;
		label_cnt++;
		emit_line("0_10101_%d_000_0_%d",d2b(pc),ecpu_cnt);
	}
	else{
		emit_line("1_10101_00000000_000_0_00000000");//dummy
	}
}
static void ecpu_emit_inst(Inst* inst) {
	ecpu_cnt++;
	if(label_ok==0)return;
  switch (inst->op) {
  case MOV:
	if(inst->src.type==REG)
			emit_line("0_00000_%s_%s",reg_names_ecpu[inst->dst.reg],reg_names_ecpu[inst->src.reg]);
	else if(inst->src.type==IMM){
			if(inst->src.imm>=0)
				emit_line("1_00000_00000%s_000_0_%d",reg_names_ecpu[inst->dst.reg],inst->src.imm);
			else
				emit_line("1_00000_%s_000_1_%d",reg_names_ecpu[inst->dst.reg],inst->src.imm);
		}
  break;

  case ADD:
	if(inst->src.type==REG){
  	  emit_line("0_00001_%s_%s&",
              reg_names_ecpu[inst->dst.reg],
               reg_names_ecpu[inst->src.reg]);
	}
	else if(inst->src.type==IMM){
			if(inst->src.imm>=0)
	    	emit_line("1_00001_00000%s_000_0_%d",reg_names_ecpu[inst->dst.reg],inst->src.imm);		
			else
	    	emit_line("1_00001_%s_000_1_%d",reg_names_ecpu[inst->dst.reg],inst->src.imm);		
	}
  break;

  case SUB:
	if(inst->src.type==REG){
  	  emit_line("0_00001_%s_%s&",
              reg_names_ecpu[inst->dst.reg],
               reg_names_ecpu[inst->src.reg]);
		}
	else if(inst->src.type==IMM){
			if(inst->src.imm>=0)
	    	emit_line("1_00010_%s_000_0_%d",
        	     reg_names_ecpu[inst->dst.reg],
          	    inst->src.imm);		
			else 
	    	emit_line("1_00010_%s_000_1_%d",
        	     reg_names_ecpu[inst->dst.reg],
          	    inst->src.imm);		
	}
    break;

  case LOAD:
		if(inst->src.type==REG)
			emit_line("0_00011_%s_%s",reg_names_ecpu[inst->dst.reg],reg_names_ecpu[inst->src.reg]);
		else if(inst->src.type==IMM)	
			emit_line("1_00011_%s_%d",reg_names_ecpu[inst->dst.reg],inst->src.imm);
  break;

  case STORE:
  	if(inst->src.type==REG)
			emit_line("0_00100_%s_%s",reg_names_ecpu[inst->dst.reg],reg_names_ecpu[inst->src.reg]);
				
		else if(inst->src.type==IMM)			
			emit_line("1_00100_%s_%d",inst->src.imm,reg_names_ecpu[inst->dst.reg]);   
  break;

  case PUTC:
		emit_line("0_00101_%s_000",reg_names_ecpu[inst->dst.reg]);
   break;

  case GETC:
    emit_line("{ int _ = getchar(); %s = _ != EOF ? _ : 0; }",
              reg_names[inst->dst.reg]);
   break;

  case EXIT:
  break;

  case DUMP:
  break;

  case EQ:
  case NE:
  case LT:
  case GT:
  case LE:
  case GE:
  case JEQ:
  case JNE:
  case JLT:
  case JGT:
  case JLE:
  case JGE:
  case JMP:
		if(inst->src.type==IMM)
			emit_line("1_%s_%s_%s_0_%s",ecpu_cmp_str(inst),ecpu_value_str(&inst->src),reg_names_ecpu[inst->dst.reg],value_str(&inst->jmp));	
		else
			emit_line("1_%s_%s_%s_0_%s",ecpu_cmp_str(inst),ecpu_value_str(&inst->src),reg_names_ecpu[inst->dst.reg],value_str(&inst->jmp));	
  break;

  default:
    error("oops");
  }
}

void target_ecpu(Module* module) {
  ecpu_init_state();
Data* data = module->data;
  for (int mp = 0; data; data = data->next, mp++) {
	  ecpu_cnt++;
    if (data->v) {
			emit_line("1_00100_%d_000_0_%d",d2b(mp),data->v);
    }
  }
  int num_funcs = emit_chunked_main_loop(module->text,
                                         ecpu_emit_func_prologue,
                                         ecpu_emit_func_epilogue,
                                         ecpu_emit_pc_change,
                                         ecpu_emit_inst);
  label_ok=1;
	emit_line("0_10110_00000000_000_0_%d",label_cnt);
	emit_line("");
   num_funcs = emit_chunked_main_loop(module->text,
                                         ecpu_emit_func_prologue,
                                         ecpu_emit_func_epilogue,
                                         ecpu_emit_pc_change,
                                         ecpu_emit_inst);
	num_funcs++;
}

