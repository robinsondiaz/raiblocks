#include <mu_coin/mu_coin.hpp>
#include <cryptopp/sha.h>

bool mu_coin::address::operator == (mu_coin::address const & other_a) const
{
    return number == other_a.number;
}

mu_coin::address::address (boost::multiprecision::uint256_t const & number_a) :
number (number_a)
{
}

mu_coin::address::address (mu_coin::EC::PublicKey const & pub) :
number (mu_coin::uint256_union (pub).number ())
{
}

CryptoPP::RandomNumberGenerator & mu_coin::pool ()
{
    static CryptoPP::AutoSeededRandomPool result;
    return result;
}

CryptoPP::OID & mu_coin::curve ()
{
    static CryptoPP::OID result (CryptoPP::ASN1::secp256k1 ());
    return result;
};

mu_coin::entry::entry (boost::multiprecision::uint256_t const & address_a, boost::multiprecision::uint256_t const & coins_a, uint8_t sequence_a) :
address (address_a),
sequence_coins (coins_a << 8 | sequence_a)
{
}

mu_coin::uint256_union::uint256_union (boost::multiprecision::uint256_t const & number_a)
{
    boost::multiprecision::uint256_t number_l (number_a);
    qwords [0] = number_l.convert_to <uint64_t> ();
    number_l >>= 64;
    qwords [1] = number_l.convert_to <uint64_t> ();
    number_l >>= 64;
    qwords [2] = number_l.convert_to <uint64_t> ();
    number_l >>= 64;
    qwords [3] = number_l.convert_to <uint64_t> ();
    std::reverse (&bytes [0], &bytes [32]);
}

mu_coin::uint512_union::uint512_union (boost::multiprecision::uint512_t const & number_a)
{
    boost::multiprecision::uint512_t number_l (number_a);
    qwords [0] = number_l.convert_to <uint64_t> ();
    number_l >>= 64;
    qwords [1] = number_l.convert_to <uint64_t> ();
    number_l >>= 64;
    qwords [2] = number_l.convert_to <uint64_t> ();
    number_l >>= 64;
    qwords [3] = number_l.convert_to <uint64_t> ();
    number_l >>= 64;
    qwords [4] = number_l.convert_to <uint64_t> ();
    number_l >>= 64;
    qwords [5] = number_l.convert_to <uint64_t> ();
    number_l >>= 64;
    qwords [6] = number_l.convert_to <uint64_t> ();
    number_l >>= 64;
    qwords [7] = number_l.convert_to <uint64_t> ();
    std::reverse (&bytes [0], &bytes [64]);
}

void mu_coin::uint256_union::clear ()
{
    bytes.fill (0);
}

void mu_coin::uint512_union::clear ()
{
    bytes.fill (0);
}

void hash_number (CryptoPP::SHA256 & hash_a, boost::multiprecision::uint256_t const & number_a)
{
    mu_coin::uint256_union bytes (number_a);
    hash_a.Update (bytes.bytes.data (), sizeof (bytes));
}

boost::multiprecision::uint256_t mu_coin::transaction_block::hash () const
{
    CryptoPP::SHA256 hash;
    mu_coin::uint256_union digest;
    for (auto i (entries.begin ()), j (entries.end ()); i != j; ++i)
    {
        hash_number (hash, i->address);
        hash_number (hash, i->sequence_coins);
    }
    hash.Final (digest.bytes.data ());
    return digest.number ();
}

boost::multiprecision::uint256_t mu_coin::uint256_union::number ()
{
    mu_coin::uint256_union temp (*this);
    std::reverse (&temp.bytes [0], &temp.bytes [32]);
    boost::multiprecision::uint256_t result (temp.qwords [3]);
    result <<= 64;
    result |= temp.qwords [2];
    result <<= 64;
    result |= temp.qwords [1];
    result <<= 64;
    result |= temp.qwords [0];
    return result;
}

boost::multiprecision::uint512_t mu_coin::uint512_union::number ()
{
    mu_coin::uint512_union temp (*this);
    std::reverse (&temp.bytes [0], &temp.bytes [64]);
    boost::multiprecision::uint512_t result (temp.qwords [7]);
    result <<= 64;
    result |= temp.qwords [6];
    result <<= 64;
    result |= temp.qwords [5];
    result <<= 64;
    result |= temp.qwords [4];
    result <<= 64;
    result |= temp.qwords [3];
    result <<= 64;
    result |= temp.qwords [2];
    result <<= 64;
    result |= temp.qwords [1];
    result <<= 64;
    result |= temp.qwords [0];
    return result;
}

boost::multiprecision::uint256_t mu_coin::transaction_block::fee () const
{
    return 1;
}

void mu_coin::entry::sign (EC::PrivateKey const & private_key, mu_coin::uint256_union const & message)
{
    EC::Signer signer (private_key);
    signer.SignMessage (pool (), message.bytes.data (), sizeof (message), signature.bytes.data ());
}

bool mu_coin::entry::validate (EC::PublicKey const & public_key, mu_coin::uint256_union const & message)
{
    EC::Verifier verifier (public_key);
    auto result (verifier.VerifyMessage (message.bytes.data (), sizeof (message), signature.bytes.data (), sizeof (signature)));
    return result;
}

mu_coin::uint256_union::uint256_union (mu_coin::EC::PublicKey const & pub)
{
    pub.GetGroupParameters ().GetCurve ().EncodePoint (bytes.data (), pub.GetPublicElement(), true);
}

mu_coin::transaction_block * mu_coin::ledger::previous (mu_coin::address const & address_a)
{
    assert (has_balance (address_a));
    auto existing (latest.find (address_a));
    return existing->second;
}

bool mu_coin::ledger::has_balance (mu_coin::address const & address_a)
{
    return latest.find (address_a) != latest.end ();
}

bool mu_coin::ledger::process (mu_coin::transaction_block * block_a)
{
    auto result (false);
    boost::multiprecision::uint256_t previous;
    boost::multiprecision::uint256_t next;
    for (auto i (block_a->entries.begin ()), j (block_a->entries.end ()); !result && i != j; ++i)
    {
        auto & address (i->address);
        auto existing (latest.find (address));
        if (i->sequence () > 0)
        {
            if (existing != latest.end ())
            {
                auto previous_entry (std::find_if (existing->second->entries.begin (), existing->second->entries.end (), [&address] (mu_coin::entry const & entry_a) {return address == entry_a.address;}));
                if (previous_entry != existing->second->entries.end ())
                {
                    if (previous_entry->sequence () + 1 == i->sequence ())
                    {
                        previous += previous_entry->coins ();
                        next += i->coins ();
                    }
                    else
                    {
                        result = true;
                    }
                }
                else
                {
                    result = true;
                }
            }
            else
            {
                result = true;
            }
        }
        else
        {
            if (existing == latest.end ())
            {
                next += i->coins ();
            }
            else
            {
                result = true;
            }
        }
    }
    if (!result)
    {
        if (next < previous)
        {
            std::string nextstr (next.convert_to<std::string>());
            std::string feestr (block_a->fee ().convert_to<std::string>());
            std::string previousstr (previous.convert_to<std::string>());
            if (next + block_a->fee () == previous)
            {
                for (auto i (block_a->entries.begin ()), j (block_a->entries.end ()); i != j; ++i)
                {
                    latest [i->address] = block_a;
                }
            }
            else
            {
                result = true;
            }
        }
        else
        {
            result = true;
        }
    }
    return result;
}

mu_coin::uint256_t mu_coin::entry::coins ()
{
    return sequence_coins >> 8;
}

uint8_t mu_coin::entry::sequence ()
{
    return sequence_coins.convert_to <uint8_t> ();
}