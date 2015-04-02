#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <string>
#include <cmath>
using namespace std;

unsigned int get_blocksize(ifstream& ifile){
    unsigned int blocksize;
    ifile.read((char*) &blocksize, sizeof(unsigned int));
    return blocksize;
}

void write_blocksize(ofstream& ofile, unsigned int blocksize){
    ofile.write((char*) &blocksize, sizeof(unsigned int));
}

class Header{
private:
    unsigned int _numpart[6];
    unsigned int _totnumpart[6];
    double _mass[6];
    char _rest1[24];
    char _rest2[136];
    char _name[4];
    unsigned int _blocksize;

public:
    void read(ifstream& ifile){
        unsigned int blocksize = get_blocksize(ifile);
        ifile.read(_name, 4);
        _blocksize = get_blocksize(ifile);
        blocksize -= get_blocksize(ifile);
        if(blocksize){
            cerr << "An error occured while reading the header" << endl;
        }
        blocksize = get_blocksize(ifile);
        ifile.read((char*) &_numpart, 6*sizeof(unsigned int));
        ifile.read((char*) &_mass, 6*sizeof(double));
        ifile.read(_rest1, 24);
        ifile.read((char*) &_totnumpart, 6*sizeof(unsigned int));
        ifile.read(_rest2, 136);
        blocksize -= get_blocksize(ifile);
        if(blocksize){
            cerr << "An error occured while reading the header" << endl;
            exit(1);
        }
    }
    
    void write(ofstream& ofile, bool header = true){
        if(header){
            write_blocksize(ofile, 8);
            ofile.write(_name, 4);
            write_blocksize(ofile, _blocksize);
            write_blocksize(ofile, 8);
        }
        write_blocksize(ofile, 256);
        ofile.write((char*) &_numpart, 6*sizeof(unsigned int));
        ofile.write((char*) &_mass, 6*sizeof(double));
        ofile.write(_rest1, 24);
        ofile.write((char*) &_totnumpart, 6*sizeof(unsigned int));
        ofile.write(_rest2, 136);
        write_blocksize(ofile, 256);
    }
    
    void get_numpart(unsigned int* npart){
        for(unsigned int i = 0; i < 6; i++){
            npart[i] = _numpart[i];
        }
    }
    
    unsigned int get_npart_mass(){
        unsigned int npart_mass = 0;
        for(unsigned int i = 0; i < 6; i++){
            if(!_mass[i]){
                npart_mass += _totnumpart[i];
            }
        }
        return npart_mass;
    }
    
    double get_mass(unsigned int index){
        return _mass[index];
    }
    
    void switch_numpart(){
        _numpart[0] = _numpart[1];
        _numpart[1] = 0;
        _mass[0] = _mass[1];
        _mass[1] = 0.;
        _totnumpart[0] = _totnumpart[1];
        _totnumpart[1] = 0;
    }
    
    void clear_masses(){
        for(unsigned int i = 0; i < 6; i++){
            _mass[i] = 0.;
        }
    }
};

class Block{
private:
    char _name[4];
    unsigned int _blocksize;
    char* _data;

public:
    Block(){
        _blocksize = 0;
    }
    
    Block(const Block& block){
        _blocksize = block._blocksize;
        for(unsigned int i = 0; i < 4; i++){
            _name[i] = block._name[i];
        }
        _data = new char[_blocksize];
        for(unsigned int i = 0; i < _blocksize; i++){
            _data[i] = block._data[i];
        }
    }
    
    ~Block(){
        if(_blocksize){
            delete [] _data;
        }
    }

    void read(ifstream& ifile){
        unsigned int blocksize = get_blocksize(ifile);
        ifile.read(_name, 4);
        string name(_name);
        _blocksize = get_blocksize(ifile);
        blocksize -= get_blocksize(ifile);
        if(blocksize){
            cerr << "An error occured while reading a block" << endl;
            exit(1);
        }
        blocksize = get_blocksize(ifile);
        _data = new char[blocksize];
        ifile.read(_data, blocksize);
        blocksize -= get_blocksize(ifile);
        if(blocksize){
            cerr << "An error occured while reading a block" << endl;
            exit(1);
        }
        _blocksize -= 2*sizeof(unsigned int);
    }
    
    void write(ofstream& ofile, bool header = true){
        if(header){
            write_blocksize(ofile, 8);
            ofile.write(_name, 4);
            write_blocksize(ofile, _blocksize+2*sizeof(unsigned int));
            write_blocksize(ofile, 8);
        }
        write_blocksize(ofile, _blocksize);
        ofile.write(_data, _blocksize);
        write_blocksize(ofile, _blocksize);
    }
    
    void fill(string name, vector<float>& data){
        for(unsigned int i = 0; i < name.size(); i++){
            _name[i] = name[i];
        }
        for(unsigned int i = name.size(); i < 4; i++){
            _name[i] = ' ';
        }
        _blocksize = sizeof(float)*data.size();
        _data = new char[_blocksize];
        for(unsigned int i = 0; i < data.size(); i++){
            for(unsigned int j = 0; j < 4; j++){
                _data[i*4+j] = *((char*) &data[i]+j);
            }
        }
    }
    
    bool is_name(string name){
        unsigned int i = 0;
        while(i < name.size() && _name[i] == name[i]){
            i++;
        }
        return !(i < name.size() || (name.size() < 4 && _name[i] != ' ' && _name[i]));
    }
    
    vector<float> get_float_data(){
        vector<float> data(_blocksize/sizeof(float), 0.);
        for(unsigned int i = 0; i < data.size(); i++){
            data[i] = *((float*) &_data[4*i]);
        }
        return data;
    }
};

class Snapshot{
private:
    Header _header;
    vector<Block> _blocks;
    
public:
    Snapshot(){
        _blocks.resize(4);
    }

    void read(string filename){
        ifstream ifile(filename.c_str(), ios::binary | ios::in);
        _header.read(ifile);
        _blocks[0].read(ifile);
        _blocks[1].read(ifile);
        _blocks[2].read(ifile);
        _blocks[3].read(ifile);
    }
    
    void write(string filename){
        ofstream ofile(filename.c_str(), ios::binary | ios::out);
        _header.write(ofile);
        for(unsigned int i = 0; i < _blocks.size(); i++){
            _blocks[i].write(ofile);
        }
    }
    
    void write_type1(string filename){
        ofstream ofile(filename.c_str(), ios::binary | ios::out);
        _header.write(ofile, false);
        for(unsigned int i = 0; i < _blocks.size(); i++){
            _blocks[i].write(ofile, false);
        }
    }
    
    void add_block(string name, vector<float>& data){
        _blocks.push_back(Block());
        _blocks.back().fill(name, data);
    }
    
    void get_numpart(unsigned int* npart){
        return _header.get_numpart(npart);
    }
    
    unsigned int get_npart_mass(){
        return _header.get_npart_mass();
    }
    
    float get_mass(unsigned int index){
        return _header.get_mass(index);
    }
    
    void switch_numpart(){
        _header.switch_numpart();
    }
    
    vector<float> get_positions(){
        unsigned int index = 0;
        while(index < _blocks.size() && !_blocks[index].is_name("POS")){
            index++;
        }
        if(index == _blocks.size()){
            cerr << "POS not found in snapshot!" << endl;
            exit(1);
        }
        return _blocks[index].get_float_data();
    }
    
    vector<float> get_masses(){
        unsigned int index = 0;
        while(index < _blocks.size() && !_blocks[index].is_name("MASS")){
            index++;
        }
        if(index == _blocks.size()){
            cerr << "MASS not found in snapshot!" << endl;
            exit(1);
        }
        return _blocks[index].get_float_data();
    }
    
    void delete_block(string name){
        unsigned int index = 0;
        while(index < _blocks.size() && !_blocks[index].is_name(name)){
            index++;
        }
        if(index == _blocks.size()){
            cerr << name << " not found in snapshot!" << endl;
            exit(1);
        }
        _blocks.erase(_blocks.begin()+index);
    }
    
    void clear_masses(){
        _header.clear_masses();
    }
};

class Particle{
private:
    double _x[3];

public:
    Particle(double x, double y, double z){
        _x[0] = x;
        _x[1] = y;
        _x[2] = z;
    }
    
    bool inside(double *c, double r){
        double d = 0.;
        for(unsigned int i = 0; i < 3; i++){
            double x = _x[i]-c[i];
            d += x*x;
        }
        return d <= r*r;
    }
    
    void add_particle(double *com){
        com[0] += _x[0];
        com[1] += _x[1];
        com[2] += _x[2];
    }
};

class ParticleSet{
private:
    vector<Particle> _particles;

public:
    void add_particle(double x, double y, double z){
        _particles.push_back(Particle(x, y, z));
    }
    
    void get_com(double *com){
        // start with an educated guess: the center of the box
        com[0] = 0.;
        com[1] = 0.;
        com[2] = 0.;
        
        // start with a very large radius: over 9000!
        double r = 9001.;
        double n = _particles.size();
        while( n > 0.1*_particles.size()){
            n = 0.;
            double newcom[3] = {0., 0., 0.};
            for(unsigned int i = 0; i < _particles.size(); i++){
                if(_particles[i].inside(com, r)){
                    _particles[i].add_particle(newcom);
                    n += 1.;
                }
            }
            com[0] = newcom[0]/n;
            com[1] = newcom[1]/n;
            com[2] = newcom[2]/n;
            r *= 0.99;
        }
    }
};

int main(int argc, char** argv){
    if(argc < 2){
        cerr << "Not enough arguments provided!" << endl;
        exit(1);
    }
    string inname(argv[1]);
    bool convert_type = false;

    Snapshot snapshot;
    snapshot.read(inname);
    unsigned int numpart[6];
    snapshot.get_numpart(numpart);
    vector<float> pos = snapshot.get_positions();
    unsigned int numpart_not_star = numpart[0]+numpart[1];
    ParticleSet particles;
    for(unsigned int i = numpart_not_star; i < pos.size()/3; i++){
        particles.add_particle(pos[3*i], pos[3*i+1], pos[3*i+2]);
    }
    double com[3];
    particles.get_com(com);
    cout << com[0] << "\t" << com[1] << "\t" << com[2] << endl;
    
    return 0;
}
