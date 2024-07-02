#include <iostream>
#include <time.h>
#include <fstream>
#include <cryptopp/aes.h>
#include "cryptopp/md5.h";
#include "cryptopp/hex.h";
#include "cryptopp/files.h";
#include "cryptopp/channels.h";
#include "gmpxx.h";
#include "gmp.h";
using namespace std;
using namespace CryptoPP;
gmp_randclass Rand(gmp_randinit_default);
class Client;
class Server {
private:
    static mpz_class C;
    static vector<int> db;
    static bool check_id(int id)
    {
        int i;
        for (i = 0; i < db.size(); i++)
        {
            if (id == db[i])
                return false;
        }
        return true;
    }
public:
    static mpz_class D;
    static mpz_class N;

    static void gen_data()
    {
        mpz_class Q, P, f, g, s, t;
        do
        {
            P = Rand.get_z_bits(1024);
        } while (mpz_probab_prime_p(P.get_mpz_t(), 40) != 1);
        do
        {
            Q = Rand.get_z_bits(1024);
        } while (mpz_probab_prime_p(Q.get_mpz_t(), 40) != 1);
        N = P * Q;
        f = (P - 1) * (Q - 1);

        do
        {
            D = Rand.get_z_range(f - 2) + 1;
            mpz_gcdext(g.get_mpz_t(), C.get_mpz_t(), t.get_mpz_t(), D.get_mpz_t(), f.get_mpz_t());
        } while (g != 1);

        cout << " N = " << N.get_str() << "\n D = " << D.get_str() << endl;
    }

    static mpz_class gen_s2(mpz_class h2, int id)
    {
        mpz_class s2;
        if (check_id(id))
        {
            db.push_back(id);
            mpz_powm(s2.get_mpz_t(), h2.get_mpz_t(), Server::C.get_mpz_t(), Server::N.get_mpz_t());
            cout << "\n Сервер:\n s2 = " << s2.get_str() << endl;
            return s2;
        }
        else
        {
            cout << " Нельзя голосовать дважды.\n";
            return -1;
        }
    }

    static void check_s(mpz_class s, mpz_class n)
    {
        mpz_class res;
        MD5 md5Hash;
        byte digest[MD5::DIGESTSIZE];
        HexEncoder encoder;
        string hash;

        md5Hash.CalculateDigest(digest, (const byte*)n.get_str().c_str(), n.get_str().length());
        encoder.Attach(new StringSink(hash));
        encoder.Put(digest, sizeof(digest));
        encoder.MessageEnd();

        mpz_class h(hash, 16);
        cout << "\n Сервер:\n hash(n) = " << h.get_str();

        mpz_powm(res.get_mpz_t(), s.get_mpz_t(), Server::D.get_mpz_t(), Server::N.get_mpz_t());
        cout << "\n s ^ d mod N = " << res.get_str() << endl;

        if (res.get_str() == h.get_str())
            cout << "\n Верно.";
        else
            cout << "\n Неверно.";
    }
};

class Client {
private:
    static mpz_class r;
public:
    static mpz_class n;
    static mpz_class gen_bul(int v)
    {
        mpz_class rnd = Rand.get_z_bits(512), res, s, t, h2;
        MD5 md5Hash;
        byte digest[MD5::DIGESTSIZE];
        HexEncoder encoder;
        string hash;

        Client::n = rnd.get_str() + to_string(v);
        md5Hash.CalculateDigest(digest, (const byte*)Client::n.get_str().c_str(), Client::n.get_str().length());
        encoder.Attach(new StringSink(hash));
        encoder.Put(digest, sizeof(digest));
        encoder.MessageEnd();
        mpz_class h(hash, 16);

        do
        {
            Client::r = Rand.get_z_range(Server::N - 2) + 1;
            mpz_gcdext(res.get_mpz_t(), s.get_mpz_t(), t.get_mpz_t(), Client::r.get_mpz_t(), Server::N.get_mpz_t());
        } while (res != 1);

        mpz_powm(res.get_mpz_t(), r.get_mpz_t(), Server::D.get_mpz_t(), Server::N.get_mpz_t());
        h2 = h * res;
        cout << "\n Сгенерирован бюллетень:\n hash = " << h.get_str() << "\n h2 = " << h2.get_str() << endl;
        return h2;
    }

    static mpz_class gen_s(mpz_class s2)
    {
        mpz_class res, s, t, rm1;
        mpz_gcdext(res.get_mpz_t(), rm1.get_mpz_t(), t.get_mpz_t(), Client::r.get_mpz_t(), Server::N.get_mpz_t());
        s = s2 * (rm1 % Server::N);
        cout << "\n Клиент:\n s = " << s.get_str() << endl;
        return s;
    }
};

mpz_class Server::D = 0, Server::N = 0, Server::C = 0, Client::r = 0, Client::n = 0;
vector<int> Server::db;
int main()
{
    setlocale(LC_ALL, "Russian");
    srand(time(0));
    int id, v;
    mpz_class h2, s2, s;
    Server::gen_data();
    while (1)
    {
        cout << "\n id: ";
        cin >> id;
        cout << " ГОЛОСОВАНИЕ:\n 1 - Да\n 2 - Нет\n 3 - Воздержался\n";
        cin >> v;
        h2 = Client::gen_bul(v);
        s2 = Server::gen_s2(h2, id);
        if (s2 != -1)
        {
            s = Client::gen_s(s2);
            Server::check_s(s, Client::n);
        }
        cout << "\n______________________\n";
    }
    return 0;
}