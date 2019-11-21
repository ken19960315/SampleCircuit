#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"

#include <iostream>
#include <fstream>
#include <ctime>
#include <unordered_map>
#include <bitset>

#include "ext-sample/SampleCircuit.h"

extern "C"{
Aig_Man_t * Abc_NtkToDar( Abc_Ntk_t * pNtk, int fExors, int fRegisters );
Abc_Ntk_t * Abc_NtkDC2( Abc_Ntk_t * pNtk, int fBalance, int fUpdateLevel, int fFanout, int fPower, int fVerbose );
Abc_Ntk_t * Abc_NtkDarSeqSweep2( Abc_Ntk_t * pNtk, Ssw_Pars_t * pPars );
Abc_Ntk_t * Abc_NtkDarFraig( Abc_Ntk_t * pNtk, int nConfLimit, int fDoSparse, int fProve, int fTransfer, int fSpeculate, int fChoicing, int fVerbose );
void Abc_NtkCecFraig( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int nSeconds, int fVerbose );
void Abc_NtkShow( Abc_Ntk_t * pNtk, int fGateNames, int fSeq, int fUseReverse );
void Abc_NtkShowBdd( Abc_Ntk_t * pNtk, int fCompl );
void Abc_NtkPrintStrSupports( Abc_Ntk_t * pNtk, int fMatrix );
Abc_Ntk_t * Abc_NtkFromGlobalBdds( Abc_Ntk_t * pNtk, int fReverse );
int Abc_NtkMinimumBase2( Abc_Ntk_t * pNtkNew );
double chisqr(int Dof, double Cv);
}

namespace
{

// optimize subroutine
Abc_Ntk_t * Ntk_Optimize(Abc_Ntk_t* pNtk)
{
    Ssw_Pars_t Pars, * pPars = &Pars;
    Ssw_ManSetDefaultParams( pPars );
    Abc_Ntk_t * pNtkOpt = Abc_NtkDup( pNtk );
    strcat(pNtkOpt->pName, "Opt");
    for (int j = 0; j < 5; j++)
    {
        if ( !Abc_NtkIsComb(pNtkOpt) )
            pNtkOpt = Abc_NtkDarSeqSweep2( pNtkOpt, pPars );
        pNtkOpt = Abc_NtkDC2( pNtkOpt, 0, 0, 1, 0, 0 );
        pNtkOpt = Abc_NtkDarFraig( pNtkOpt, 100, 1, 0, 0, 0, 0, 0 );
    }    

    return pNtkOpt;
}

// ABC command: Testing of generating sample circuits 
int SampleGenTest_Command( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    char c;
    int nPI = 0;
    int nPO = 0;
    int times = 1;
    double mAnd = 0, mAndOpt = 0;
    char* filename;
    bool f_verbose = false;
    bool f_redirect = false;
    fstream file;
    Abc_Ntk_t *pAig, *pAigOpt;
    SampleCircuit sc;

    // parse arguments
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "ionrvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'i':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-i\" should be followed by an integer.\n" );
                goto usage;
            }
            nPI = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( nPI <= 0 )
                goto usage;
            break;
        case 'o':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-o\" should be followed by an integer.\n" );
                goto usage;
            }
            nPO = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( nPO <= 0 )
                goto usage;
            break;
        case 'n':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-n\" should be followed by an integer.\n" );
                goto usage;
            }
            times = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( times <= 0 )
                goto usage;
            break;
        case 'r':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-r\" should be followed by an string.\n" );
                goto usage;
            }
            f_redirect = true;
            filename = argv[globalUtilOptind];
            globalUtilOptind++;
            break;
        case 'v':
            f_verbose = true;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }

    // check if the number of PO/PI is available
    if (nPI <= 0 || nPO <= 0)
    {
        Abc_Print( -1, "The number of PI/PO should be larger than 0.\n" );
        goto usage;
    }
    if (nPI >= nPO)
    {
        Abc_Print( -1, "The number of PI should be less than the number of PO.\n" );
        goto usage;
    }

    // generate sample circuit
    if (f_verbose)
        Abc_Print( -2, "Generate sample circuit w/ nPI = %d and nPO = %d %d times\n", nPI, nPO, times );
    sc.setIOnum(nPI, nPO);
    sc.setRndSeed((unsigned)time(NULL));
    for (int i = 0; i < times; i++)
    {    
        pAig = sc.genCircuit();
        //if (f_verbose)
            //cout << sc;
        mAnd += Abc_NtkNodeNum(pAig);
        //pAigOpt = Ntk_Optimize(pAig);
        //mAndOpt += Abc_NtkNodeNum(pAigOpt);
        Abc_NtkDelete( pAig );
        //Abc_NtkDelete( pAigOpt );
    }
    mAnd /= times;
    //mAndOpt /= times;

    if (f_redirect)
    {
        file.open(filename, ios::out|ios::trunc);
        assert(file.is_open());
        file << nPI << " " << nPO << "\n";
        file << mAnd << "\n";
        //file << mAndOpt << "\n";
        file.close();
    }
    //if (f_verbose)
        //Abc_Print( -2, "Avg. AIG size: %.2f, Avg. Opt. AIG size: %.2f\n", mAnd, mAndOpt );

    return 0;

usage:
    Abc_Print( -2, "usage: sampleGenTest [-i <num>] [-o <num>] [-n <num>] [-r <file>] [-vh]\n" );
    Abc_Print( -2, "\t        Generate a sample circuit with given PI and PO number\n" );
    Abc_Print( -2, "\t-i <num> : sets the number of PI\n");
    Abc_Print( -2, "\t-o <num> : sets the number of PO\n");
    Abc_Print( -2, "\t-n <num> : sets the test times\n");
    Abc_Print( -2, "\t-r <file>: redirect to given file\n");
    Abc_Print( -2, "\t-v       : verbose\n");
    Abc_Print( -2, "\t-h       : print the command usage\n");
    return 0;
}
// ABC command: Generate a sample circuit 
int SampleGen_Command( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    char c;
    int nPI = 0;
    int nPO = 0;
    bool f_verbose = false;
    Abc_Ntk_t *pAig;
    SampleCircuit sc;

    // parse arguments
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "iovh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'i':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-i\" should be followed by an integer.\n" );
                goto usage;
            }
            nPI = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( nPI <= 0 )
                goto usage;
            break;
        case 'o':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-o\" should be followed by an integer.\n" );
                goto usage;
            }
            nPO = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( nPO <= 0 )
                goto usage;
            break;
        case 'v':
            f_verbose = true;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }

    // check if the number of PO/PI is available
    if (nPI <= 0 || nPO <= 0)
    {
        Abc_Print( -1, "The number of PI/PO should be larger than 0.\n" );
        goto usage;
    }
    if (nPI >= nPO)
    {
        Abc_Print( -1, "The number of PI should be less than the number of PO.\n" );
        goto usage;
    }

    // generate sample circuit
    Abc_Print( -2, "Generate sample circuit w/ nPI = %d, nPO = %d\n", nPI, nPO );
    sc.setIOnum(nPI, nPO);
    sc.setRndSeed((unsigned)time(NULL));
    pAig = sc.genCircuit();
    if (f_verbose)
        cout << sc;

    // replace the current network
    Abc_FrameReplaceCurrentNetwork(pAbc, pAig);

    return 0;

usage:
    Abc_Print( -2, "usage: sampleGen [-ioh]\n" );
    Abc_Print( -2, "\t        Generate a sample circuit with given PI and PO number\n" );
    Abc_Print( -2, "\t-i <num> : sets the number of PI\n");
    Abc_Print( -2, "\t-o <num> : sets the number of PO\n");
    Abc_Print( -2, "\t-v       : verbose\n");
    Abc_Print( -2, "\t-h       : print the command usage\n");
    return 0;
}

// ABC command: Generate a sample circuit and connect to current Aig
int SampleCnt_Command( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    char c;
    int nPI = 0;
    int nPO;
    bool f_verbose = false;
    bool f_auto = false;
    bool f_cone = false;
    Abc_Ntk_t * pNtk;
    Abc_Ntk_t * pAig, * pAigNew; 
    SampleCircuit sc;

    // parse arguments
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "iacvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'i':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-i\" should be followed by an integer.\n" );
                goto usage;
            }
            nPI = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( nPI <= 0 )
                goto usage;
            break;
        case 'v':
            f_verbose = true;
            break;
        case 'a':
            f_auto = true;
            break;
        case 'c':
            f_cone = true;
            break;
        case 'h':
        default:
            goto usage;
        }
    }

    // get the current network
    pNtk = Abc_FrameReadNtk(pAbc);
    if (pNtk == NULL)
    {
        Abc_Print( -1, "Cannot get the current network.\n");
        goto usage;
    }
    assert(Abc_NtkHasAig(pNtk) && Abc_NtkIsStrash(pNtk));
    if (f_verbose)
    {
        Abc_Print( -2, "Network Information\n");
        Abc_Print( -2, "Pi Num: %d\n", Abc_NtkPiNum(pNtk));
        Abc_Print( -2, "Ci Num: %d\n", Abc_NtkCiNum(pNtk));
        Abc_Print( -2, "Box Num: %d\n", Abc_NtkBoxNum(pNtk));
        Abc_Print( -2, "Latch Num: %d\n", Abc_NtkLatchNum(pNtk));
    }
    nPO = Abc_NtkPiNum(pNtk);
    if (nPO == 1)
    {
        Abc_Print( -1, "Circuit has only one PI.\n");
        return 0;
    }
    if (f_auto) 
        nPI = (nPO+1)/2;

    // check if the number of PO/PI is available
    if (nPI <= 0)
    {
        Abc_Print( -1, "The number of PI should be larger than 0.\n" );
        goto usage;
    }
    if (nPI >= nPO)
    {
        Abc_Print( 0, "The number of PI should be less than the number of PO.\n" );
        Abc_Print( -2, "Automatically set to %d.\n", (nPO+1)/2 );
        nPI = (nPO+1)/2;
    }

    // generate sample circuit & connect
    if (f_verbose)
        Abc_Print( -2, "Generate sample circuit w/ nPI = %d, nPO = %d\n", nPI, nPO );
    sc.setIOnum(nPI, nPO);
    sc.setRndSeed((unsigned)time(NULL));
    if (f_cone)
        pAig = sc.genCircuit(pNtk);
    else
        pAig = sc.genCircuit();
    pAigNew = sc.connect(pNtk);
    if (f_verbose)
    {
        Abc_Print( -2, "Sampling Circuit:\n" );
        Abc_NtkPrintStrSupports( pAig, 0 );
        Abc_Print( -2, "Connect to current network\n");
    }
    if (f_verbose)
    {
        Abc_Print( -2, "Original Network:\n" );
        Abc_NtkPrintStrSupports( pNtk, 0 );
        Abc_Print( -2, "Sampling Network:\n" );
        Abc_NtkPrintStrSupports( pAigNew, 0 );
    }

    // replace the current network
    Abc_FrameReplaceCurrentNetwork(pAbc, pAigNew);

    return 0;

usage:
    Abc_Print( -2, "usage: sampleCnt [-iavh]\n" );
    Abc_Print( -2, "\t        Generate a sample circuit with given PI number and connect it to current AIG network\n" );
    Abc_Print( -2, "\t-i <num> : sets the number of PI\n");
    Abc_Print( -2, "\t-a       : sets the number of PI equal to the half number of PO of current network\n");
    Abc_Print( -2, "\t-c        : sampling for each cone and consider the correlation\n");
    Abc_Print( -2, "\t-v       : verbose\n");
    Abc_Print( -2, "\t-h       : print the command usage\n");
    return 0;
}

// ABC command: Test BDD size after sampling
int SampleBddTest_Command( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    char c;
    int i, j;
    int nPI = 0;
    int nPO;
    int nCo;
    int times = 0;
    double mAnd1  = 0.0, mAnd2  = 0.0;
    double mNode1 = 0.0, mNode2 = 0.0;
    bool f_redirect = false;
    bool f_verbose = false;
    bool f_auto = false;
    bool f_correlation = false;
    bool f_sort = false;
    fstream file;
    char* filename;
    Abc_Ntk_t * pNtk, * pNtkOpt, * pNtkRes;
    Abc_Ntk_t * pAigNew; 
    SampleCircuit sc;
    DdManager * dd1, * dd2;
    abctime clk = Abc_Clock();
    Abc_Obj_t * pNodeCo;
    vector<int>    vCoAnd1, vCoBdd1;
    vector<double> vCoAnd2, vCoBdd2;

    // parse arguments
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "inracsvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'i':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-i\" should be followed by an integer.\n" );
                goto usage;
            }
            nPI = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( nPI <= 0 )
                goto usage;
            break;
        case 'n':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-n\" should be followed by an integer.\n" );
                goto usage;
            }
            times = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( times <= 0 )
                goto usage;
            break;
        case 'r':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-r\" should be followed by an string.\n" );
                goto usage;
            }
            f_redirect = true;
            filename = argv[globalUtilOptind];
            globalUtilOptind++;
            break;
        case 'v':
            f_verbose = true;
            break;
        case 'a':
            f_auto = true;
            break;
        case 'c':
            f_correlation = true;
            break;
        case 's':
            f_sort = true;
            break;
        case 'h':
        default:
            goto usage;
        }
    }
    if (f_correlation && f_sort)
        goto usage;

    // get the current network
    pNtk = Abc_FrameReadNtk(pAbc);
    if (pNtk == NULL)
    {
        Abc_Print( -1, "Cannot get the current network.\n");
        return 0;
    }
    // check whether the network is combinational
    if (!Abc_NtkIsComb(pNtk))
    {
        Abc_Print( -1, "Network is not combinational.\n" );
        return 0;
    }
    assert(Abc_NtkHasAig(pNtk) && Abc_NtkIsStrash(pNtk));
    nPO = Abc_NtkPiNum(pNtk);
    if (nPO == 1)
    {
        Abc_Print( -1, "Circuit has only one PI.\n");
        return 0;
    }
    if (f_auto) 
        nPI = (nPO+1)/2;

    // check if the number of PO/PI is available
    if (nPI <= 0)
    {
        Abc_Print( -1, "The number of PI should be larger than 0.\n" );
        return 0;
    }
    if (nPI >= nPO)
    {
        Abc_Print( 0, "The number of PI should be less than the number of PO.\n" );
        return 0;
    }
   
    // optimize original network
    pNtkOpt = Ntk_Optimize(pNtk);
    if (f_verbose)
    {
        Abc_Print( -2, "Optimized. Before: %d, After: %d\n", Abc_NtkNodeNum(pNtk), Abc_NtkNodeNum(pNtkOpt) );
        Abc_NtkCecFraig( pNtk, pNtkOpt, 20, 0 );
    }

    // size of each cone
    nCo = Abc_NtkPoNum(pNtkOpt);
    vCoAnd1.resize(nCo);
    vCoBdd1.resize(nCo);
    vCoAnd2.resize(nCo, 0);
    vCoBdd2.resize(nCo, 0);
    for (i = 0; i < nCo; i++)
    {
        pNodeCo = Abc_NtkCo( pNtkOpt, i );
        pNtkRes = Abc_NtkCreateCone( pNtkOpt, Abc_ObjFanin0(pNodeCo), Abc_ObjName(pNodeCo), 0 );
        if ( pNodeCo && Abc_ObjFaninC0(pNodeCo) )
            Abc_NtkPo(pNtkRes, 0)->fCompl0 ^= 1;
        vCoAnd1[i] = Abc_NtkNodeNum(pNtkRes);
        Abc_NtkBuildGlobalBdds(pNtkRes, ABC_INFINITY, 1, 1, 0, 0);
        dd2 = (DdManager *)Abc_NtkGlobalBddMan( pNtkRes );
        vCoBdd1[i] = Cudd_ReadNodeCount(dd2);
        Abc_NtkFreeGlobalBdds( pNtkRes, 1 ); 
        Abc_NtkDelete( pNtkRes );
    }
    
    // collapse optimized network
    //pNtkRes = Abc_NtkCollapse(pNtkOpt, ABC_INFINITY, 0, 1, 0, 0);
    clk = Abc_Clock();
    assert( Abc_NtkIsStrash(pNtkOpt) );
    if ( Abc_NtkBuildGlobalBdds(pNtkOpt, ABC_INFINITY, 1, 1, 0, 0) == NULL )
    {
        Abc_Print( -1, "BDD constructed failed.\n");
        return 0;
    }
    dd1 = (DdManager *)Abc_NtkGlobalBddMan( pNtkOpt );
    if ( f_verbose )
    {
        ABC_PRT( "BDD construction time", Abc_Clock() - clk );
        Abc_Print( -2, "BDD order: " );
        for (i = 0; i < dd1->size; i++)
            Abc_Print( -2, "%d ", Cudd_ReadPerm(dd1, i) );
        Abc_Print( -2, "\n" );
    }
    //cout << Abc_NtkGetBddNodeNum(pNtkRes) << "\n";
    //cout << Cudd_ReadKeys(dd1) - Cudd_ReadDead(dd1) << "\n";
    //cout << Cudd_ReadNodeCount(dd1) << "\n";
    mAnd1 = Abc_NtkNodeNum(pNtkOpt);
    mNode1 = Cudd_ReadNodeCount(dd1);
    
    Abc_NtkFreeGlobalBdds( pNtkOpt, 1 ); 
    Abc_NtkDelete( pNtkOpt );

    // generate sample circuit & connect
    assert(times > 0);
    if (f_verbose)
        Abc_Print( -2, "\nGenerate sample circuit w/ nPI = %d, nPO = %d, and %d times.\n", nPI, nPO , times );
    sc.setIOnum(nPI, nPO);
    sc.setRndSeed((unsigned)time(NULL));
    
    // collapse sampling network
    for (i = 0; i < times; i++)
    {
        if (f_correlation)
            sc.genCircuit( pNtk );
        else if (f_sort)
            sc.genCircuit2( pNtk );
        else
            sc.genCircuit();
        if (f_verbose)
            cout << sc;
        pAigNew = sc.connect( pNtk );
    
        // optimize sampling circuit
        pNtkOpt = Ntk_Optimize( pAigNew );
        
        //
        for (j = 0; j < nCo; j++)
        {
            pNodeCo = Abc_NtkCo( pNtkOpt, j );
            pNtkRes = Abc_NtkCreateCone( pNtkOpt, Abc_ObjFanin0(pNodeCo), Abc_ObjName(pNodeCo), 0 );
            if ( pNodeCo && Abc_ObjFaninC0(pNodeCo) )
                Abc_NtkPo(pNtkRes, 0)->fCompl0 ^= 1;
            vCoAnd2[j] += Abc_NtkNodeNum(pNtkRes);
            Abc_NtkBuildGlobalBdds(pNtkRes, ABC_INFINITY, 1, 1, 0, 0);
            dd2 = (DdManager *)Abc_NtkGlobalBddMan( pNtkRes );
            vCoBdd2[j] += Cudd_ReadNodeCount(dd2);
            Abc_NtkFreeGlobalBdds( pNtkRes, 1 ); 
            Abc_NtkDelete( pNtkRes );
        }

        if (f_verbose)
        {
            Abc_Print( -2, "Sampling (%d/%d)\nOptimized. Before: %d, After: %d\n", i+1, times, Abc_NtkNodeNum(pAigNew), Abc_NtkNodeNum(pNtkOpt) );
            Abc_NtkCecFraig( pAigNew, pNtkOpt, 20, 0 );
        }
        
        // collapse optimized network
        //pNtkRes = Abc_NtkCollapse(pNtkOpt, ABC_INFINITY, 0, 1, 0, 0);
        clk = Abc_Clock();
        assert( Abc_NtkIsStrash(pNtkOpt) );
        if ( Abc_NtkBuildGlobalBdds(pNtkOpt, ABC_INFINITY, 1, 1, 0, 0) == NULL )
        {
            Abc_Print( -1, "BDD constructed failed.\n");
            return 0;
        }
        dd2 = (DdManager *)Abc_NtkGlobalBddMan( pNtkOpt );
        if ( f_verbose )
        {
            ABC_PRT( "BDD construction time", Abc_Clock() - clk );
            Abc_Print( -2, "BDD order: " );
            for (j = 0; j < dd2->size; j++)
                Abc_Print( -2, "%d ", Cudd_ReadPerm(dd2, j) );
            Abc_Print( -2, "\n" );
        }
        
        mAnd2 += Abc_NtkNodeNum(pNtkOpt);
        mNode2 += Cudd_ReadNodeCount(dd2);
        Abc_NtkFreeGlobalBdds( pNtkOpt, 1 ); 
        Abc_NtkDelete( pAigNew );
        Abc_NtkDelete( pNtkOpt );
    }
    mAnd2 /= times;
    mNode2 /= times;
    for (i = 0; i < nCo; i++)
    {
        vCoAnd2[i] /= times;
        vCoBdd2[i] /= times;
    }

    // redirect bdd information
    if (f_redirect)
    {
        file.open(filename, ios::out|ios::trunc);
        assert(file.is_open());
        file << Abc_NtkPiNum(pNtk) << " " << Abc_NtkPoNum(pNtk) << "\n";
        file << mAnd1 << " " << mNode1 << "\n";
        file << mAnd2 << " " << mNode2 << "\n";
        for (i = 0; i < nCo; i++)
            file << vCoAnd1[i] << " ";
        file << "\n";
        for (i = 0; i < nCo; i++)
            file << vCoBdd1[i] << " ";
        file << "\n";
        for (i = 0; i < nCo; i++)
            file << vCoAnd2[i] << " ";
        file << "\n";
        for (i = 0; i < nCo; i++)
            file << vCoBdd2[i] << " ";
        file << "\n";
        file.close();
    }
    
    // print the result
    if (!f_redirect)
    {
        Abc_Print( -2, "\nOrigin:\n\tANDs: %.0f, BDDs: %.0f\n", mAnd1, mNode1 );
        Abc_Print( -2, "\tANDs for Each Cone: " );
        for (i = 0; i < nCo; i++)
            Abc_Print( -2, "%d ", vCoAnd1[i]);
        Abc_Print( -2, "\n" );
        Abc_Print( -2, "\tBDDs for Each Cone: " );
        for (i = 0; i < nCo; i++)
            Abc_Print( -2, "%d ", vCoBdd1[i]);
        Abc_Print( -2, "\n" );
        Abc_Print( -2, "Sample:\n\tMean ANDs: %.2f, Mean BDDs: %.2f\n", mAnd2, mNode2 );
        Abc_Print( -2, "\tANDs for Each Cone: " );
        for (i = 0; i < nCo; i++)
            Abc_Print( -2, "%.2f ", vCoAnd2[i]);
        Abc_Print( -2, "\n" );
        Abc_Print( -2, "\tBDDs for Each Cone: " );
        for (i = 0; i < nCo; i++)
            Abc_Print( -2, "%.2f ", vCoBdd2[i]);
        Abc_Print( -2, "\n" );
    }

    return 0;

usage:
    Abc_Print( -2, "usage: sampleBddTest [-i <num>] [-n <num>] [-r <file>] [-acsvh]\n" );
    Abc_Print( -2, "\t        Test BDD size after sampling with several times\n" );
    Abc_Print( -2, "\t-i <num>  : sets the number of PI\n");
    Abc_Print( -2, "\t-n <num>  : sets the test times\n");
    Abc_Print( -2, "\t-r <file> : redirect to given file\n");
    Abc_Print( -2, "\t-a        : sets the number of PI equal to the half number of PO of current network\n");
    Abc_Print( -2, "\t-c        : sampling for each cone and consider the correlation\n");
    Abc_Print( -2, "\t-s        : sort the XOR and support\n");
    Abc_Print( -2, "\t-v        : verbose\n");
    Abc_Print( -2, "\t-h        : print the command usage\n");
    return 0;
}

// ABC command: Test BDD size after sampling
int SampleBddTest2_Command( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    char c;
    int i, j, k;
    int nPI = 0;
    int nPO, nPORes;
    int times = 0;
    double mAnd1  = 0.0, mAnd2  = 0.0;
    double mNode1 = 0.0, mNode2 = 0.0;
    double mIll = 0.0;
    bool f_redirect = false;
    bool f_verbose = false;
    bool f_auto = false;
    fstream file;
    char* filename;
    Abc_Obj_t * pNodeCo, * pPi;
    Abc_Ntk_t * pNtk, * pNtkOpt, * pNtkRes;
    Abc_Ntk_t * pAigNew; 
    SampleCircuit sc;
    DdManager * dd1, * dd2;
    abctime clk = Abc_Clock();
    unordered_map<string, int> PImap;

    // parse arguments
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "inravh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'i':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-i\" should be followed by an integer.\n" );
                goto usage;
            }
            nPI = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( nPI <= 0 )
                goto usage;
            break;
        case 'n':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-n\" should be followed by an integer.\n" );
                goto usage;
            }
            times = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( times <= 0 )
                goto usage;
            break;
        case 'r':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-r\" should be followed by an string.\n" );
                goto usage;
            }
            f_redirect = true;
            filename = argv[globalUtilOptind];
            globalUtilOptind++;
            break;
        case 'v':
            f_verbose = true;
            break;
        case 'a':
            f_auto = true;
            break;
        case 'h':
        default:
            goto usage;
        }
    }

    // get the current network
    pNtk = Abc_FrameReadNtk(pAbc);
    if (pNtk == NULL)
    {
        Abc_Print( -1, "Cannot get the current network.\n");
        return 0;
    }
    // check whether the network is combinational
    if (!Abc_NtkIsComb(pNtk))
    {
        Abc_Print( -1, "Network is not combinational.\n" );
        return 0;
    }
    assert(Abc_NtkHasAig(pNtk) && Abc_NtkIsStrash(pNtk));
    nPO = Abc_NtkPiNum(pNtk);
    if (nPO == 1)
    {
        Abc_Print( -1, "Circuit has only one PI.\n");
        return 0;
    }
    if (f_auto) 
        nPI = (nPO+1)/2;

    // check if the number of PO/PI is available
    if (nPI <= 0)
    {
        Abc_Print( -1, "The number of PI should be larger than 0.\n" );
        return 0;
    }
    if (nPI >= nPO)
    {
        Abc_Print( 0, "The number of PI should be less than the number of PO.\n" );
        return 0;
    }
   
    // optimize original network
    pNtkOpt = Ntk_Optimize(pNtk);
    if (f_verbose)
    {
        Abc_Print( -2, "Optimized. Before: %d, After: %d\n", Abc_NtkNodeNum(pNtk), Abc_NtkNodeNum(pNtkOpt) );
        Abc_NtkCecFraig( pNtk, pNtkOpt, 20, 0 );
    }
    
    // collapse optimized network
    //pNtkRes = Abc_NtkCollapse(pNtkOpt, ABC_INFINITY, 0, 1, 0, 0);
    clk = Abc_Clock();
    assert( Abc_NtkIsStrash(pNtkOpt) );
    if ( Abc_NtkBuildGlobalBdds(pNtkOpt, ABC_INFINITY, 1, 1, 0, 0) == NULL )
    {
        Abc_Print( -1, "BDD constructed failed.\n");
        return 0;
    }
    dd1 = (DdManager *)Abc_NtkGlobalBddMan( pNtkOpt );
    if ( f_verbose )
    {
        ABC_PRT( "BDD construction time", Abc_Clock() - clk );
        Abc_Print( -2, "BDD order: " );
        for (i = 0; i < dd1->size; i++)
            Abc_Print( -2, "%d ", Cudd_ReadPerm(dd1, i) );
        Abc_Print( -2, "\n" );
    }
    //cout << Abc_NtkGetBddNodeNum(pNtkRes) << "\n";
    //cout << Cudd_ReadKeys(dd1) - Cudd_ReadDead(dd1) << "\n";
    //cout << Cudd_ReadNodeCount(dd1) << "\n";
    mAnd1 += Abc_NtkNodeNum(pNtkOpt);
    mNode1 += Cudd_ReadNodeCount(dd1);
    
    Abc_NtkFreeGlobalBdds( pNtkOpt, 1 ); 
    Abc_NtkDelete( pNtkOpt );

    // generate sample circuit & connect
    assert(times > 0);
    if (f_verbose)
        Abc_Print( -2, "\nGenerate sample circuit w/ nPI = %d, nPO = %d, and %d times.\n", nPI, nPO, times );
    sc.setRndSeed((unsigned)time(NULL));
   
    // construct mapping between ID and name for current network
    Abc_NtkForEachPi( pNtk, pPi, k )
    {
        PImap[Abc_ObjName(pPi)] = k;
        if (f_verbose)
            cout << "Map " << Abc_ObjName(pPi) << " to " << PImap[Abc_ObjName(pPi)] << "\n";
    }
    if (f_verbose)
        cout << "\n";
    // collapse sampling network
    for (i = 0; i < times; i++)
    {
        if (f_verbose)
            Abc_Print( -2, "\nSampling...(%d/%d)\n", i+1, times );
        vector<bool> check_vec(pow(2,nPI), true);
        vector<string> str_vec(pow(2,nPI), string(nPO, '-'));
        for (j = 0; j < Abc_NtkPoNum(pNtk); j++)
        {
            pNodeCo = Abc_NtkCo( pNtk, j );
            pNtkRes = Abc_NtkCreateCone( pNtk, Abc_ObjFanin0(pNodeCo), Abc_ObjName(pNodeCo), 0 );
            if ( pNodeCo && Abc_ObjFaninC0(pNodeCo) )
                Abc_NtkPo(pNtkRes, 0)->fCompl0 ^= 1;
            
            if (f_verbose)
            {
                Abc_Print( -2, "Cone %d:\n", j );
                Abc_Print( -2, "Support: " );
                Abc_NtkForEachPi( pNtkRes, pPi, k )
                    Abc_Print( -2, "%s ", Abc_ObjName(pPi) );
                Abc_Print( -2, "\n");
            }
            //cout << Abc_NtkPiNum(pNtkRes) << "\n";
            
            nPORes = Abc_NtkPiNum(pNtkRes);
            if (nPORes > nPI)
            {
                sc.setIOnum(nPI, nPORes);
                sc.genCircuit();
                if (f_verbose)
                    cout << sc;
                // check patterns
                vector< vector<int> > XOR = sc.getXOR();
                for (k = 0; k < pow(2,nPI); k++)
                {
                    if (!check_vec[k]) continue;
                    string binary = bitset<20>(k).to_string();
                    //cout << "str_vec: " << str_vec[k] << "\n";
                    //cout << "binary: " << binary << "\n";
                    //cout << "out: ";
                    for (int m = 0; m < nPORes; m++)
                    {
                        int output = XOR[m][0];
                        for (int n = 1; n < XOR[m].size(); n++)
                        {
                            if (binary[binary.size()-XOR[m][n]-1] == '1')
                                output ^= 1;
                        }
                        //cout << output;
                        int pos = PImap[Abc_ObjName(Abc_NtkPi(pNtkRes, m))];
                        if (str_vec[k][pos] == '-')
                            str_vec[k][pos] = (char)(output+48);
                        else if (str_vec[k][pos] != (char)(output+48))
                        {
                            check_vec[k] = false;
                            break;
                        }
                    }
                    //cout << "\n";
                    //cout << "str_vec: " << str_vec[k] << "\n\n";
                }
                pAigNew = sc.connect( pNtkRes );
                
                // optimize sampling circuit
                pNtkOpt = Ntk_Optimize( pAigNew );
                if (f_verbose)
                {
                    Abc_Print( -2, "Logic network optimized. Before: %d, After: %d\n", Abc_NtkNodeNum(pAigNew), Abc_NtkNodeNum(pNtkOpt) );
                    Abc_NtkCecFraig( pAigNew, pNtkOpt, 20, 0 );
                }
            }
            else
            {
                if (f_verbose)
                {
                    for (k = 0; k < nPORes; k++)
                        Abc_Print( -2, "\tPO%d = PI%d\n", PImap[Abc_ObjName(Abc_NtkPi(pNtkRes, k))], k );
                }
                // check patterns
                for (k = 0; k < pow(2,nPI); k++)
                {
                    if (!check_vec[k]) continue;
                    string binary = bitset<20>(k).to_string();
                    //cout << "str_vec: " << str_vec[k] << "\n";
                    //cout << "binary: " << binary << "\n";
                    for (int m = 0; m < nPORes; m++)
                    {
                        int pos = PImap[Abc_ObjName(Abc_NtkPi(pNtkRes, m))];
                        if (str_vec[k][pos] == '-')
                            str_vec[k][pos] = binary[binary.size()-m-1];
                        else if (str_vec[k][pos] != binary[binary.size()-m-1])
                        {
                            check_vec[k] = false;
                            break;
                        }
                    }
                    //cout << "str_vec: " << str_vec[k] << "\n\n";
                }
                pNtkOpt = Ntk_Optimize( pNtkRes );
                if (f_verbose)
                {
                    Abc_Print( -2, "Logic network optimized. Before: %d, After: %d\n", Abc_NtkNodeNum(pNtkRes), Abc_NtkNodeNum(pNtkOpt) );
                    Abc_NtkCecFraig( pNtkRes, pNtkOpt, 20, 0 );
                }
            }

            // collapse optimized network
            clk = Abc_Clock();
            assert( Abc_NtkIsStrash(pNtkOpt) );
            if ( Abc_NtkBuildGlobalBdds(pNtkOpt, ABC_INFINITY, 1, 1, 0, 0) == NULL )
            {
                Abc_Print( -1, "BDD constructed failed.\n");
                return 0;
            }
            dd2 = (DdManager *)Abc_NtkGlobalBddMan( pNtkOpt );
            if ( f_verbose )
            {
                ABC_PRT( "BDD construction time", Abc_Clock() - clk );
                Abc_Print( -2, "BDD order: " );
                for (k = 0; k < dd2->size; k++)
                    Abc_Print( -2, "%d ", Cudd_ReadPerm(dd2, k) );
                Abc_Print( -2, "\n" );
            }
            
            mAnd2 += Abc_NtkNodeNum(pNtkOpt);
            mNode2 += Cudd_ReadNodeCount(dd2);
            Abc_NtkFreeGlobalBdds( pNtkOpt, 1 ); 
            if (nPORes > nPI) Abc_NtkDelete( pAigNew );
            Abc_NtkDelete( pNtkOpt );
            Abc_NtkDelete( pNtkRes );
        }
        for (j = 0; j < check_vec.size(); j++)
            if (!check_vec[j]) mIll++;
    }
    mAnd2 /= times;
    mNode2 /= times;
    mIll /= (times*pow(2, nPI));
    
    // redirect bdd information
    if (f_redirect)
    {
        file.open(filename, ios::out|ios::trunc);
        assert(file.is_open());
        file << Abc_NtkPiNum(pNtk) << " " << Abc_NtkPoNum(pNtk) << "\n";
        file << mAnd1 << " " << mNode1 << "\n";
        file << mAnd2 << " " << mNode2 << " " << mIll << "\n";
        file.close();
    }
   
    if (f_verbose)
    {
        Abc_Print( -2, "\nOrigin:\n\tGates: %.2f, Nodes: %.2f\n", mAnd1, mNode1 );
        Abc_Print( -2, "Sample:\n\tMean Gates: %.2f, Mean Nodes: %.2f, Illegal rate: %.2f\n", mAnd2, mNode2, mIll );
    }

    return 0;

usage:
    Abc_Print( -2, "usage: sampleBddTest2 [-i <num>] [-n <num>] [-r <file>] [-avh]\n" );
    Abc_Print( -2, "\t        Test BDD size after sampling with several times\n" );
    Abc_Print( -2, "\t-i <num>  : sets the number of PI\n");
    Abc_Print( -2, "\t-n <num>  : sets the test times\n");
    Abc_Print( -2, "\t-r <file> : redirect to given file\n");
    Abc_Print( -2, "\t-a        : sets the number of PI equal to the half number of PO of current network\n");
    Abc_Print( -2, "\t-v        : verbose\n");
    Abc_Print( -2, "\t-h        : print the command usage\n");
    return 0;
}

// ABC command: Pearson Chi Square Test for sampling circuit
int SampleChiTest_Command( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    char c;
    int i, j, pos;
    int nPI = 0;
    int nPO = 0;
    int times = 0;
    bool f_correlation = false;
    bool f_redirect = false;
    bool f_verbose = false;
    fstream file;
    char* filename;
    Abc_Obj_t * pPi;
    Abc_Ntk_t * pNtk;
    SampleCircuit sc;
    unordered_map<string, int> PImap;
    int expect = 5;
    vector<short> vObserve;
    double valCrit, valP;

    // parse arguments
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "ioecrvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'i':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-i\" should be followed by an integer.\n" );
                goto usage;
            }
            nPI = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( nPI <= 0 )
                goto usage;
            break;
        case 'o':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-o\" should be followed by an integer.\n" );
                goto usage;
            }
            nPO = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( nPO <= 0 )
                goto usage;
            break;
        case 'e':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-e\" should be followed by an integer.\n" );
                goto usage;
            }
            expect = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( expect <= 0 )
                goto usage;
            break;
        case 'r':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-r\" should be followed by an string.\n" );
                goto usage;
            }
            f_redirect = true;
            filename = argv[globalUtilOptind];
            globalUtilOptind++;
            break;
        case 'v':
            f_verbose = true;
            break;
        case 'c':
            f_correlation = true;
            break;
        case 'h':
        default:
            goto usage;
        }
    }

    // get the current network
    if (f_correlation)
    {
        pNtk = Abc_FrameReadNtk(pAbc);
        if (pNtk == NULL)
        {
            Abc_Print( -1, "Cannot get the current network.\n");
            return 0;
        }
        // check whether the network is combinational
        if (!Abc_NtkIsComb(pNtk))
        {
            Abc_Print( -1, "Network is not combinational.\n" );
            return 0;
        }
        assert(Abc_NtkHasAig(pNtk) && Abc_NtkIsStrash(pNtk));
        nPO = Abc_NtkPiNum(pNtk);
    
        // construct mapping between ID and name for current network
        Abc_NtkForEachPi( pNtk, pPi, i )
        {
            PImap[Abc_ObjName(pPi)] = i;
            if (f_verbose)
                cout << "Map " << Abc_ObjName(pPi) << " to " << PImap[Abc_ObjName(pPi)] << "\n";
        }
        if (f_verbose)
            cout << "\n";
    }

    // check if the number of PO/PI is available
    if (nPO == 1)
    {
        Abc_Print( -1, "Circuit has only one PI.\n");
        return 0;
    }
    if (nPI <= 0)
    {
        Abc_Print( -1, "The number of PI should be larger than 0.\n" );
        return 0;
    }
    if (nPI >= nPO)
    {
        Abc_Print( -1, "The number of PI should be less than the number of PO.\n" );
        return 0;
    }
    if (nPO > 20)
    {
        Abc_Print( -1, "The number of PO should be less than 20 due to memory concern.\n" );
        return 0;
    }

    // generate sampling circuit
    times = expect*pow(2,nPO) / pow(2,nPI);
    if (f_verbose)
        Abc_Print( -2, "\nGenerate sample circuit w/ nPI = %d, nPO = %d, and %d times.\n", nPI, nPO, times );
    sc.setRndSeed((unsigned)time(NULL));
    vObserve.resize(pow(2,nPO));
    for (i = 0; i < pow(2,nPO); i++)
        vObserve[i] = 0;
    for (i = 0; i < times; i++)
    {
        if (f_verbose)
            Abc_Print( -2, "\nSampling...(%d/%d)\n", i+1, times );
            
        sc.setIOnum(nPI, nPO);
        sc.genCircuit();
        if (f_verbose)
            cout << sc;

        // count patterns
        vector< vector<int> > XOR = sc.getXOR();
        for (j = 0; j < pow(2,nPI); j++)
        {
            string in = bitset<20>(j).to_string();
            string out(nPO, '-');
            for (int m = 0; m < nPO; m++)
            {
                int output = XOR[m][0];
                for (int n = 1; n < XOR[m].size(); n++)
                {
                    if (in[in.size()-XOR[m][n]-1] == '1')
                        output ^= 1;
                }
                pos = f_correlation? PImap[Abc_ObjName(Abc_NtkPi(pNtk, m))] : m;
                out[pos] = (char)(output+48);
            }
            vObserve[bitset<20>(out).to_ulong()]++;
        }
    }

    // compute chi-square value
    valCrit = 0;
    for (i = 0; i < pow(2,nPO); i++)
    {
        double XSqr = vObserve[i] - expect;
        valCrit += ((XSqr * XSqr) / expect);
    }
    
    // compute p-value
    valP = chisqr(pow(2,nPO)-1, valCrit);
    cout << "DoF = " << pow(2,nPO)-1 << "\n";
    cout << "Chi-value = " << valCrit << "\n";
    cout << "P-value = " << valP << "\n";

    /*// redirect information
    if (f_redirect)
    {
        file.open(filename, ios::out|ios::trunc);
        assert(file.is_open());
        file << Abc_NtkPiNum(pNtk) << " " << Abc_NtkPoNum(pNtk) << "\n";
        file << mAnd1 << " " << mNode1 << "\n";
        file << mAnd2 << " " << mNode2 << " " << mIll << "\n";
        file.close();
    }*/

    return 0;

usage:
    Abc_Print( -2, "usage: sampleChiTest [-i <num>] [-o <num>] [-r <file>] [-cvh]\n" );
    Abc_Print( -2, "\t        Apply Pearson Chi-Square test on sampling circuit\n" );
    Abc_Print( -2, "\t-i <num>  : sets the number of PI\n");
    Abc_Print( -2, "\t-o <num>  : sets the number of PO\n");
    Abc_Print( -2, "\t-e <num>  : sets the expected number of each pattern[default:5]\n");
    Abc_Print( -2, "\t-r <file> : redirect to given file\n");
    Abc_Print( -2, "\t-c        : consider network supports correlation\n");
    Abc_Print( -2, "\t-v        : verbose\n");
    Abc_Print( -2, "\t-h        : print the command usage\n");
    return 0;
}
// called during ABC startup
void init(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd( pAbc, "Sample", "sampleGen", SampleGen_Command, 0);
    Cmd_CommandAdd( pAbc, "Sample", "sampleCnt", SampleCnt_Command, 0);
    Cmd_CommandAdd( pAbc, "Sample", "sampleGenTest", SampleGenTest_Command, 0);
    Cmd_CommandAdd( pAbc, "Sample", "sampleBddTest", SampleBddTest_Command, 0);
    Cmd_CommandAdd( pAbc, "Sample", "sampleBddTest2", SampleBddTest2_Command, 0);
    Cmd_CommandAdd( pAbc, "Sample", "sampleChiTest", SampleChiTest_Command, 0);
}

// called during ABC termination
void destroy(Abc_Frame_t* pAbc)
{
}

// this object should not be modified after the call to Abc_FrameAddInitializer
Abc_FrameInitializer_t frame_initializer = { init, destroy };

// register the initializer a constructor of a global object
// called before main (and ABC startup)
struct registrar
{
    registrar() 
    {
        Abc_FrameAddInitializer(&frame_initializer);
    }
} sample_registrar;

} // unnamed namespace
