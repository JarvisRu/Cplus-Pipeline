/**
=====================================================================
利用C++實作PipeLine處理器之指令運作模擬程式
先將 IFID_Reg 、 IDEX_Reg 、 EXMEM_Reg 、 MEMWB_Reg 分別建立為物件後
再以物件:Pipeline 包住上面四個Reg
---------------------------------------------------------------------
實作下列指令：
R-type:add、sub、and、or 、slt
I-type:lw、sw、addi、andi、bne
---------------------------------------------------------------------
並且能夠偵測與處理
1.data hazard
2.hazard with load
3.branch hazard
---------------------------------------------------------------------
最多僅能一次執行20個指令
=====================================================================
*/



#include<iostream>
#include<fstream>
#include<cstdlib>
#include<math.h>

using namespace std;

void initialize();            // for initialize register
void general();               // for General
void data_Hazard();           // for Data_Hazard
void lu_Hazard();             // for LU_Hazard
void branch_Hazrd();          // for Branch_Hazard
bool lu_haz = false;

int convert2decimal(bool sign_ex,string str);

ifstream fin;       // for read file
ofstream fout;      // for write file

string all_Instructions[21];
void clear_instr();

int reg[10] = {0,9,8,7,1,2,3,4,5,6};
int mem[5] = {5,9,4,8,7};

/**-------------------IF/ID Reg--------------------------------*/
struct IFID_Reg{
    int pc;
    string instr;

    IFID_Reg(){
        pc = 0;
        instr = "00000000000000000000000000000000" ;
    }

    void fetch_Code(){
        pc = pc+4;
        // fetch code from all
        if(all_Instructions[pc/4] != ""){
            instr = all_Instructions[pc/4];
        }
        // if no instruction in
        else{
            instr = "00000000000000000000000000000000" ;
        }
    }

    void flush(){
        pc = pc-4;
        instr = "00000000000000000000000000000000" ;
    }

};

/**-------------------ID/EX Reg--------------------------------*/
struct IDEX_Reg{
    int pc;
    string instr;

    string op_Code,func_Code;
    string control_Signals;
    int ReadData1,ReadData2;
    int sign_ext;
    int Rs,Rt,Rd;

    IDEX_Reg(){
        ReadData1 = 0;
        ReadData2 = 0;
        sign_ext = 0;
        Rs = 0;
        Rt = 0;
        Rd = 0;
        pc = 0;
        control_Signals = "000000000";
    }

    void decode(IFID_Reg &this_ifid){
        pc = this_ifid.pc;
        /** break down the instr */
        if(this_ifid.instr=="00000000000000000000000000000000"){
            ReadData1 = 0;
            ReadData2 = 0;
            sign_ext = 0;
            Rs = 0;
            Rt = 0;
            Rd = 0;
            pc = 0;
            control_Signals = "000000000";
        }
        else{
            op_Code  = this_ifid.instr.substr(0,6);
            Rs       = convert2decimal(false,this_ifid.instr.substr(6,5));
            Rt       = convert2decimal(false,this_ifid.instr.substr(11,5));
            Rd       = convert2decimal(false,this_ifid.instr.substr(16,5));
            sign_ext = convert2decimal(true,this_ifid.instr.substr(16,16));
            ReadData1 = reg[Rs];
            ReadData2 = reg[Rt];
            /** According to what kind of instr to set control Signals*/
            func_Code = this_ifid.instr.substr(26,6);
            // R-type
            if(op_Code=="000000"){
                if(func_Code!="000000"){
                    control_Signals = "110000010";
                }
                else{   // nop
                    control_Signals = "000000000";
                }
            }
            // I-type
            else{
                Rd = 0;

                if(op_Code=="100011"){      // lw
                    control_Signals = "000101011";
                }
                else if(op_Code=="101011"){ // sw
                    control_Signals = "00010010X";
                }
                else if(op_Code=="001000"){ // addi
                    control_Signals = "000100010";
                }
                else if(op_Code=="001100"){ // andi
                    control_Signals = "011100010";
                }
                else if(op_Code=="000101"){ // bne
                    control_Signals = "001010000";
                }
            }
        }
        // if load-use hazard
        if(lu_haz == true){
            control_Signals = "000000000";
            lu_haz = false;
        }
    }

    void flush(){
        ReadData1 = 0;
        ReadData2 = 0;
        sign_ext = 0;
        Rs = 0;
        Rt = 0;
        Rd = 0;
        pc = 0;
        control_Signals = "000000000";
    }
};

/**-------------------EXE/MEM Reg--------------------------------*/
struct EXEMEM_Reg{
    int pc;
    string instr;

    string control_Signals;
    string ALUop;
    int in1,in2,ALUout;
    int writeData;
    int Rt;

    bool branch;

    EXEMEM_Reg(){
        Rt = 0;
        pc = 0;
        ALUout = 0;
        writeData = 0;
        control_Signals = "00000";
        branch = false;
    }

    void execute(IDEX_Reg &this_idex){
        pc = this_idex.pc+(this_idex.sign_ext*4);
        branch = false;
        // get control signals
        control_Signals = this_idex.control_Signals.substr(4,5);
        ALUop = this_idex.control_Signals.substr(1,2);

        /** Get what ALU 2 input data should be */
        in1 = this_idex.ReadData1;
        // ALUop1 = 1 --> R-type , bne
        if(ALUop[0]=='1'||ALUop=="01"){
            in2 = this_idex.ReadData2;
        }
        // ALUop1 = 0 --> andi addi
        else{
            in2 = this_idex.sign_ext;
        }

        /** Get destination */
        // R-type
        if(this_idex.control_Signals[0]=='1'){
            Rt = this_idex.Rd;
        }
        // others
        else{
            Rt = this_idex.Rt;
        }

        /** Calculate ALUout */
        // R-type
        if(ALUop=="10"){
            //add
            if(this_idex.func_Code=="100000"){
                ALUout = in1 + in2;
            }
            //sub
            else if (this_idex.func_Code=="100010"){
                ALUout = in1 - in2;
            }
            //and
            else if (this_idex.func_Code=="100100"){
                ALUout = in1 & in2;
            }
            //or
            else if (this_idex.func_Code=="100101"){
                ALUout = in1 | in2;
            }
            //slt
            else if (this_idex.func_Code=="101010"){
                if(in2-in1>0)
                    ALUout = 1;
                else
                    ALUout = 0;
            }
            // nop
            else
                ALUout = 0;
        }
        // lw sw addi
        else if(ALUop=="00"){
            ALUout = in1 + in2;
        }
        // bne
        else if(ALUop=="01"){
            ALUout = in1 - in2;
            /** if branch ---> branch hazard*/
            if(ALUout!=0){
                branch = true;
            }
        }
        // andi
        else if(ALUop=="11"){
            ALUout = in1 & in2;
        }
        // set writeData
        if(this_idex.control_Signals!="000000000")
            writeData = reg[this_idex.Rt];
        else
            writeData = 0;
    }

};

/**-------------------MEM/WB Reg--------------------------------*/
struct MEMWB_Reg{
    int pc;
    string instr;

    string control_Signals;
    string ALUop;
    int readData;
    int Rt,ALUout;

    bool input;

    MEMWB_Reg(){
        readData = 0;
        Rt = 0;
        ALUout = 0;
        control_Signals = "00000";
        ALUop = "00";
    }

    void memory_RW(EXEMEM_Reg this_exemem , IFID_Reg &this_ifid){

        control_Signals = this_exemem.control_Signals.substr(3,2);
        Rt = this_exemem.Rt;
        ALUout = this_exemem.ALUout;
        ALUop = this_exemem.ALUop;

        /** set data to memory */
        // lw
        if(control_Signals=="11" && ALUop=="00"){
            readData = mem[ALUout/4];
        }
        //sw
        else if(control_Signals=="0X" && ALUop=="00"){
            mem[ALUout/4] = this_exemem.writeData;
        }
        else
            readData = 0;
    }
};


/**-------------------Pipeline--------------------------------*/
struct Pipeline{
    int clock_Cycle;
    IFID_Reg    ifid;
    IDEX_Reg    idex;
    EXEMEM_Reg  exemem;
    MEMWB_Reg   memwb;

    Pipeline(){
        clock_Cycle = 0;
    }

    void writeback_Reg(MEMWB_Reg writeback){
        /** If write back */
        if(writeback.control_Signals[0]=='1'){
            // if want to write into $0
            if(writeback.Rt == 0)
                return;
            // R-type
            if(writeback.control_Signals[1]=='0'){
                reg[writeback.Rt] = writeback.ALUout;
            }
            // lw
            else{
                reg[writeback.Rt] = writeback.readData;
            }
        }
    }

    // set all data to next stage
    bool nextStep(){
        writeback_Reg(memwb);
        memwb.memory_RW(exemem, ifid);
        exemem.execute(idex);
        idex.decode(ifid);
        ifid.fetch_Code();
        clock_Cycle++;

        // detect hazard
        if(exemem.branch){
            detect_Hazard();
            printNow();
        }
        else{
            printNow();
            detect_Hazard();
        }

        // need to go on ?
        if(idex.control_Signals=="000000000" && exemem.control_Signals=="00000"
           && memwb.control_Signals=="00" && memwb.ALUout==0 && memwb.readData==0 && clock_Cycle!=1){
            return false;
        }
        else
            return true;
    }

    // detect hazard
    void detect_Hazard(){
            /** choose ReadData1 by detect data hazard for forwardingA */
            // forwardingA = 01
            if(memwb.Rt==idex.Rs && memwb.control_Signals[0]=='1' && memwb.Rt!=0){
                if(memwb.control_Signals[1]=='1')
                    idex.ReadData1 = memwb.readData;
                else
                    idex.ReadData1 = memwb.ALUout;
                cout<<"Data Hazard happen! MEMWB data Hazard  , forwardingA = 01"<<endl<<endl;
            }
            // forwardingA = 10
            else if(exemem.Rt==idex.Rs && exemem.control_Signals[3]=='1' && exemem.Rt!=0){
                idex.ReadData1 = exemem.ALUout;
                cout<<"Data Hazard happen! EXEMEM data Hazard , forwardingA = 10"<<endl<<endl;
            }
            /** choose ReadData2 by detect data hazard for forwardingB */
            // forwardingB = 01
            if(memwb.Rt==idex.Rt && memwb.control_Signals[0]=='1' && memwb.Rt!=0){
                if(memwb.control_Signals[1]=='1')
                    idex.ReadData2 = memwb.readData;
                else
                    idex.ReadData2 = memwb.ALUout;
                cout<<"Data Hazard happen! MEMWB data Hazard  , forwardingB = 01"<<endl<<endl;
            }
            // forwardingB = 10
            else if(exemem.Rt==idex.Rt && exemem.control_Signals[3]=='1' && exemem.Rt!=0){
                idex.ReadData2 = exemem.ALUout;
                cout<<"Data Hazard happen! EXEMEM data Hazard , forwardingB = 10"<<endl<<endl;
            }

            /** Detect if Load-Use hazard
            在此有一點跟助教所給的輸出範例不同 不同在於nop的情況
            因為每一層的進行會由上一層的control signal傳下且做邏輯判斷
            因此在輸出範例中CC4之ID/EX的control signal全為0的情況下
            CC5的ALUout、writeData、Rt則不會進行如公開測資之運算
            但一切都不影響最後結果 (因為不會writeBack) */
            if(idex.control_Signals[5]=='1' && ( idex.Rt == convert2decimal(false,ifid.instr.substr(6,5))|| idex.Rt == convert2decimal(false,ifid.instr.substr(11,5)))){
                ifid.pc = ifid.pc-4;
                lu_haz = true;
                cout<<"Load-Use Hazard happen!!"<<endl<<endl;
            }

            /** Detect if Branch hazard */
            if(exemem.branch){
                ifid.pc = exemem.pc;
                ifid.fetch_Code();
                idex.flush();
                cout<<"Branch Hazard happen!!"<<endl<<endl;
            }
    }

    // write now status into file
    void printNow(){
        fout<<"CC "<<clock_Cycle<<":"<<endl<<endl;

        fout<<"Registers:"<<endl;
        fout<<"$0: "<<reg[0]<<endl;
        fout<<"$1: "<<reg[1]<<endl;
        fout<<"$2: "<<reg[2]<<endl;
        fout<<"$3: "<<reg[3]<<endl;
        fout<<"$4: "<<reg[4]<<endl;
        fout<<"$5: "<<reg[5]<<endl;
        fout<<"$6: "<<reg[6]<<endl;
        fout<<"$7: "<<reg[7]<<endl;
        fout<<"$8: "<<reg[8]<<endl;
        fout<<"$9: "<<reg[9]<<endl<<endl;

        fout<<"Data memory:"<<endl;
        fout<<"0x00: "<<mem[0]<<endl;
        fout<<"0x04: "<<mem[1]<<endl;
        fout<<"0x08: "<<mem[2]<<endl;
        fout<<"0x0C: "<<mem[3]<<endl;
        fout<<"0x10: "<<mem[4]<<endl<<endl;

        fout<<"IF/ID:"<<endl;
        fout<<"PC:              "<<ifid.pc<<endl;
        fout<<"Instruction:     "<<ifid.instr<<endl<<endl;

        fout<<"ID/EX :"<<endl;
        fout<<"ReadData1        "<<idex.ReadData1<<endl;
        fout<<"ReadData2        "<<idex.ReadData2<<endl;
        fout<<"sign_ext         "<<idex.sign_ext<<endl;
        fout<<"Rs               "<<idex.Rs<<endl;
        fout<<"Rt               "<<idex.Rt<<endl;
        fout<<"Rd               "<<idex.Rd<<endl;
        fout<<"Control signals  "<<idex.control_Signals<<endl<<endl;

        fout<<"EX/MEM :"<<endl;
        fout<<"ALUout           "<<exemem.ALUout<<endl;
        fout<<"WriteData        "<<exemem.writeData<<endl;
        fout<<"Rt/Rd            "<<exemem.Rt<<endl;
        fout<<"Control signals  "<<exemem.control_Signals<<endl<<endl;

        fout<<"MEM/WB :"<<endl;
        fout<<"Read Data        "<<memwb.readData<<endl;
        fout<<"ALUout           "<<memwb.ALUout<<endl;
        fout<<"Rt/Rd            "<<memwb.Rt<<endl;
        fout<<"Control signals  "<<memwb.control_Signals<<endl;
        fout<<"========================================================"<<endl;
    }

};

// create object Pipeline
Pipeline first_Pipeline;
Pipeline second_Pipeline;
Pipeline third_Pipeline;
Pipeline fourth_Pipeline;

/**-------------------Main--------------------------------*/

int main(){
    cout<<"================== General Case =========================="<<endl<<endl;
    general();

    cout<<"================== Data Hazard Case ======================"<<endl<<endl;
    data_Hazard();

    cout<<"================== Load Use Hazard Case =================="<<endl<<endl;
    lu_Hazard();

    cout<<"================== Branch Hazard Case ===================="<<endl<<endl;
    branch_Hazrd();

    cout<<"=========================================================="<<endl<<endl;
    return 0;
}

void initialize(){
    reg[0] = 0;
    reg[1] = 9;
    reg[2] = 8;
    reg[3] = 7;
    reg[4] = 1;
    reg[5] = 2;
    reg[6] = 3;
    reg[7] = 4;
    reg[8] = 5;
    reg[9] = 6;

    mem[0] = 5;
    mem[1] = 9;
    mem[2] = 4;
    mem[3] = 8;
    mem[4] = 7;

}

void general(){
    // prepare foe read and write file
    initialize();
    fin.open("General.txt",ifstream::in);
    if(!fin){
        cout<<"Can't read file!!"<<endl;;
        return;
    }
    fout.open("genResult.txt",ifstream::out);
    if(!fout){
        cout<<"Can't write file!!"<<endl;;
        return;
    }

    // get all instruction
    int n;
    n = 1;
    while(fin>>all_Instructions[n])
        n++;

    // do the pipeline
    while(first_Pipeline.nextStep());
    cout<<"Mission complete!"<<endl;
    cout<<"Read General.txt and write result into genResult.txt"<<endl<<endl;
    // file close
    fin.close();
    fout.close();
}

void data_Hazard(){
    // prepare foe read and write file
    initialize();
    clear_instr();
    fin.open("Datahazard.txt",ifstream::in);
    if(!fin){
        cout<<"Can't read file!!"<<endl;;
        return;
    }
    fout.open("dataResult.txt",ifstream::out);
    if(!fout){
        cout<<"Can't write file!!"<<endl;;
        return;
    }

    // get all instruction
    int n;
    n = 1;
    while(fin>>all_Instructions[n])
        n++;

    // do the pipeline
    while(second_Pipeline.nextStep());
    cout<<"Mission complete!"<<endl;
    cout<<"Read Datahazard.txt and write result into dataResult.txt"<<endl<<endl;
    // file close
    fin.close();
    fout.close();
}

void lu_Hazard(){
    // prepare foe read and write file
    initialize();
    clear_instr();
    fin.open("Lwhazard.txt",ifstream::in);
    if(!fin){
        cout<<"Can't read file!!"<<endl;;
        return;
    }
    fout.open("loadResult.txt",ifstream::out);
    if(!fout){
        cout<<"Can't write file!!"<<endl;;
        return;
    }

    // get all instruction
    int n;
    n = 1;
    while(fin>>all_Instructions[n])
        n++;

    // do the pipeline
    while(third_Pipeline.nextStep());
    cout<<"Mission complete!"<<endl;
    cout<<"Read Lwhazard.txt and write result into loadResult.txt"<<endl<<endl;
    // file close
    fin.close();
    fout.close();
}

void branch_Hazrd(){
    // prepare foe read and write file
    initialize();
    clear_instr();
    fin.open("Branchhazard.txt",ifstream::in);
    if(!fin){
        cout<<"Can't read file!!"<<endl;;
        return;
    }
    fout.open("branchResult.txt",ifstream::out);
    if(!fout){
        cout<<"Can't write file!!"<<endl;;
        return;
    }

    // get all instruction
    int n;
    n = 1;
    while(fin>>all_Instructions[n])
        n++;

    // do the pipeline
    while(fourth_Pipeline.nextStep());
    cout<<"Mission complete!"<<endl;
    cout<<"Read Branchhazard.txt and write result into branchResult.txt"<<endl<<endl;
    // file close
    fin.close();
    fout.close();
}

/** binary string to decimal */
int convert2decimal( bool sign_ex,string str){
    bool flag = false;
    if (str[0] == '1')
        flag = true;
    int res = 0;

    for (int i = str.length()-1 , a = 0; i >0 ; a++ ,i--){
        res += (str[i]-'0')*pow(2,a);
    }
    if (flag && sign_ex){               //for sign_ext
        res = -res;
    }
    return res;
}

/** clear instr array */
void clear_instr(){
    for(int n = 1 ; n <= 20 ; n++){
        all_Instructions[n]="";
    }
}
