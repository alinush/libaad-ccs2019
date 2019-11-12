#include <aad/Configuration.h>

#include <aad/PublicParameters.h>

namespace libaad {

PublicParameters::PublicParameters(const std::string& trapFile, int maxQ, bool progress, bool verify) {
    ifstream tin(trapFile);
    if(tin.fail()) {
        throw std::runtime_error("Could not open trapdoor file for reading");
    }

    loginfo << "Reading back q-PKE parameters... (verify = " << verify << ")" << endl;

    tin >> s;
    tin >> tau;
    tin >> q;       // we read the q before so as to preallocate the std::vectors
    libff::consume_OUTPUT_NEWLINE(tin);
    tin >> g2tau;

    loginfo << "q = " << q << ", s = " << s << ", tau = " << tau << endl;

    if(maxQ >= 0 && static_cast<size_t>(maxQ) > q) {
        logerror << "You asked to read " << maxQ << " public parameters, but there are only " << q << " in the '" << trapFile << "-*' files" << endl;
        throw std::runtime_error("maxQ needs to be <= q");
    }

    if(tin.fail() || tin.bad()) {
        throw std::runtime_error("Error reading full trapdoor file");
    }
    testAssertEqual(g2tau, tau * G2::one());
    
    tin.close();

    q = maxQ <= 0 ? q : static_cast<size_t>(maxQ);
    resize(q);
        
    G1 g1 = G1::one();
    G2 g2 = G2::one();
    Fr si = Fr::one();  // will store s^i
    Fr tausi = tau;     // will store \tau s^i

    size_t i = 0;
    int iter = 0;
    G1 tmpG1si;
    int prevPct = -1;
    while(i <= q) {
        std::string inFile = trapFile + "-" + std::to_string(iter);
        loginfo << q - i << " params left to read, looking at " << inFile << " ..." << endl;
        ifstream fin(inFile);

        if(fin.fail()) {
            throw std::runtime_error("Could not open public parameters file for reading");
        }
        
        size_t line = 0;
        while(i <= q) {
            line++;

            fin >> tmpG1si;
            if(fin.eof()) {
                logdbg << "Reached end of '" << inFile << "' at line " << line << endl;
                break;
            }

            g1si[i] = tmpG1si;
            libff::consume_OUTPUT_NEWLINE(fin);
            fin >> g1tausi[i];
            libff::consume_OUTPUT_NEWLINE(fin);
            fin >> g2si[i];
            libff::consume_OUTPUT_NEWLINE(fin);
            //fin >> g2tausi[i];
            //libff::consume_OUTPUT_NEWLINE(fin);

            // Check what we're reading against trapdoors

            //logtrace << g1si[i] << endl;
            //logtrace << si*g1 << endl << endl;

            // Fully verify the parameters, if verify is true
            if(verify) {
                testAssertEqual(g1si[i], si*g1);
                testAssertEqual(g1tausi[i], tausi*g1);
                testAssertEqual(ReducedPairing(g1si[i], g2tau), ReducedPairing(g1tausi[i], G2::one()));
                testAssertEqual(g2si[i], si*g2);
                //testAssertEqual(g2tausi[i], tausi*g2);
            }
        
            if(progress) {
                //int pct = static_cast<int>(static_cast<double>(c)/static_cast<double>(endExcl-startIncl) * 10000.0);
                int pct = static_cast<int>(static_cast<double>(i)/static_cast<double>(q+1) * 100.0);
                if(pct > prevPct) {
                    //loginfo << pct/100.0 << "% ... (i = " << i << " out of " << endExcl-1 << ")" << endl;
                    loginfo << pct << "% ... (i = " << i << " out of " << q+1 << ")" << endl;
                    prevPct = pct;
                    
                    // Occasionally check the parameters
                    testAssertEqual(g1si[i], si*g1);
                    testAssertEqual(g1tausi[i], tausi*g1);
                    testAssertEqual(ReducedPairing(g1si[i], g2tau), ReducedPairing(g1tausi[i], G2::one()));
                    testAssertEqual(g2si[i], si*g2);
                }
            }

            si = si * s;
            tausi = tausi * s;
            i++;

        }

        fin.close();
        iter++; // move to the next file
    }

    if(i < q) {
        throw std::runtime_error("Did not read all parameters.");
    }

    if(i != q+1) {
        throw std::runtime_error("Expected to read exactly q parameters.");
    }
}

void PublicParameters::regenerateTrapdoors(std::string& trapFile) {
    Fr s, tau;
    G2 g2tau;
    size_t q;

    ifstream tin(trapFile);
    if(tin.fail()) {
        throw std::runtime_error("Could not open trapdoor file for reading");
    }

    tin >> s;
    if(tin.fail() || tin.bad()) {
        throw std::runtime_error("Could not read trapdoor 's'");
    }
    tin >> tau;
    if(tin.fail() || tin.bad()) {
        throw std::runtime_error("Could not read trapdoor 'tau'");
    }
    tin >> q;       // we read the q before so as to preallocate the std::vectors
    if(tin.fail() || tin.bad()) {
        throw std::runtime_error("Could not read q");
    }
    libff::consume_OUTPUT_NEWLINE(tin);
    tin >> g2tau;

    if(tin.fail() || tin.bad()) {
        logwarn << "Regenerating trapdoors file because it's missing g2^{tau}..." << endl;
        tin.close();

        ofstream fout(trapFile);
        if(fout.fail()) {
            throw std::runtime_error("Could not open trapdoor file for writing");
        }

        fout << s << endl;
        fout << tau << endl;
        fout << q << endl;
        fout << tau * G2::one() << endl;
    } else {
        loginfo << "File has g2^{tau}, no need to regenerate!" << endl;
        testAssertEqual(g2tau, tau * G2::one());
        tin.close();
    }
}

std::tuple<Fr, Fr> PublicParameters::generateTrapdoors(size_t q, const std::string& outFile)
{
    ofstream fout(outFile);

    if(fout.fail()) {
        throw std::runtime_error("Could not open trapdoors file for writing");
    }

    // pick trapdoors s and \tau
    Fr s = Fr::random_element();
    Fr tau = Fr::random_element();

    loginfo << "Writing q-PKE trapdoors..." << endl;
    loginfo << "q = " << q << ", s = " << s << ", tau = " << tau << endl;
    // WARNING: SECURITY: Insecure implementation currently outputs the
    // trapdoors!
    fout << s << endl;
    fout << tau << endl;
    fout << q << endl;
    fout << tau * G2::one() << endl;

    fout.close();

    return std::make_tuple(s, tau);
}

void PublicParameters::generate(size_t startIncl, size_t endExcl, const Fr& s, const Fr& tau, const std::string& outFile, bool progress)
{
    if(startIncl >= endExcl) {
        logwarn << "Nothing to generate in range [" << startIncl << ", " << endExcl << ")" << endl;
        return;
    }

    ofstream fout(outFile);

    if(fout.fail()) {
        throw std::runtime_error("Could not open public parameters file for writing");
    }

    G1 g1tau = tau * G1::one();

    //logdbg << "i \\in [" << startIncl << ", " << endExcl << "), s = " << s << ", tau = " << tau << endl;

    // generate and write g1, g1^\tau, g2, g2^\tau 
    Fr si = s ^ startIncl;

    // generate q-PKE powers and write to file
    int prevPct = -1;
    size_t c = 0;
    for (size_t i = startIncl; i < endExcl; i++) {
        G1 g1si = si * G1::one();
        G1 g1tausi = si * (g1tau);
        G2 g2si = si * G2::one();
        //G2 g2tausi = si * (tau * G2::one());

        fout << g1si << "\n";
        fout << g1tausi << "\n";
        fout << g2si << "\n";
        //fout << g2tausi << "\n";
    
        si *= s;

        if(progress) {
            //int pct = static_cast<int>(static_cast<double>(c)/static_cast<double>(endExcl-startIncl) * 10000.0);
            int pct = static_cast<int>(static_cast<double>(c)/static_cast<double>(endExcl-startIncl) * 100.0);
            if(pct > prevPct) {
                //loginfo << pct/100.0 << "% ... (i = " << i << " out of " << endExcl-1 << ")" << endl;
                loginfo << pct << "% ... (i = " << i << " out of " << endExcl-1 << ")" << endl;
                prevPct = pct;
                
                fout << std::flush;
            }
        }

        c++;
    }
    
    fout.close();
}

}
