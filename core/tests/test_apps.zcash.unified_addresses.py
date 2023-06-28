from common import *

from apps.common import coininfo
from apps.zcash import unified_addresses

P2PKH = unified_addresses.Typecode.P2PKH
P2SH = unified_addresses.Typecode.P2SH
SAPLING = unified_addresses.Typecode.SAPLING
ORCHARD = unified_addresses.Typecode.ORCHARD

TESTVECTORS = [
    [
        "From https://github.com/zcash-hackworks/zcash-test-vectors/blob/master/unified_address.py"
    ],
    [
        "p2pkh_bytes, p2sh_bytes, sapling_raw_addr, orchard_raw_addr, unknown_typecode, unknown_bytes, unified_addr, root_seed, account, diversifier_index"
    ],
    [
        "e6cabf813929132d772d04b03ae85223d03b9be8",
        None,
        None,
        "d4714ee761d1ae823b6972152e20957fefa3f6e3129ea4dfb0a9e98703a63dab929589d6dc51c970f935b3",
        65533,
        "f6ee6921481cdd86b3cc4318d9614fc820905d042bb1ef9ca3f24988c7b3534201cfb1cd8dbf69b8250c18ef41294ca97993db546c1fe0",
        "753179793677386e336a6d6a73676a39777663656e7238723570366833387679636c686d71307767396b7a70786c7534367a387636346b3567737a72387966777a346a7672796c76766733673633337a30326c756b38356e6d73636b366432736578336e3564376b6e3638687a7a3574763475647439703673793770676c6565756c76676c767832363237646666353771396665703577676478386d3065737832386d307a767578706d7779617a74336a756e3272707177386e75366a326663657167686b353563656436366a73366b366a786e387932787475653866337061716a726b3871366e70746e6e",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        0,
        0,
    ],
    [
        "7bec9de217c04f7ce1a86f1fb458aa881c8f39e4",
        None,
        None,
        "d8e5ecb4e005c28718e61a5c336a4f369e771ccdb3363f4f7a04b02a966901a4c05da662d5fd75678f7fb4",
        65530,
        None,
        "75317a35677538783364766b7677636d726a30716b3568727839706361646c3536683834663777647970366e7635337233643563636365646563686d77393835746765357733633272353639716137326c676775753578727178683739616a7a63376b716d65733230706b747a71726a6c707835367168676d716d3536686e39777432686379787064616d616b",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        1,
        0,
    ],
    [
        "aa6d43480fd9d91375ce6c4a020706361bd296de",
        None,
        "88533c398a49c2513dc85162bf220abaf47dc983f14e908ddaaa7322dba16531bc62efe750fe575c8d149b",
        None,
        65530,
        None,
        "7531343367706a3772643934766d39356d7a73757537746a74716161677934706d6678386c6b77656d70786a7463777a33357a746361383530796e6c7a323932307477617a6171703270367168787878337a357178616b6e73716372676c7578716a337070757367776635757963686c61677938376b376874613768773965793336776d7930367065776c6470",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        2,
        0,
    ],
    [
        None,
        "a8d7551db5fd9313e8c7203d996af7d477083756",
        "52fd6aedefbf401633c2e4532515ebcf95bcc2b4b8e4d676dfad7e17925c6dfb8671e52544dc2ca075e261",
        None,
        65534,
        None,
        "753178797970646a307a7978637466666b6878796d766a6e6b376e383371666c376e7365356c3071726b346e3266376465376c3733727a79787970347463727975356d6b7875617a6c646e633279306479747a7567797a79636739373034616a66786173376b63757761776d706877776e383839743938743735376579716667346a766566746b687672337167",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        3,
        0,
    ],
    [
        None,
        "f44ab023752cb5b406ed8985e18130ab33362697",
        None,
        "165082de84f2ad7204426ffafd6b6c7de9cab6d25c13846a1786715268c415948db788f4a5e0daa03d699e",
        65533,
        None,
        "7531706a336c72656d6e7175737368393878667161336a66647077303872726b35377330346b6c32366865707a7133746a72736e78653574367371716567653976716d776c63366c786373746e6333306e3575357232776b6b7a687039367a3564306a797530716137746b686378366663386a35396b616b387a35636570363261716d61336d36343566683863",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        4,
        0,
    ],
    [
        None,
        None,
        None,
        "ea9df83fbee07d6f7895ebb2ea41ec7c4ba682b863e069b4a438e31c9571c83126c305d75456412aeaef1b",
        65531,
        None,
        "753132787567643930666c726b646b6575336e6c6e6e337565736b793533707175356d323479366170786d38386d34387637333734636c7335367a7039336e61796c617864636866307161796678747267653034376d393533717a3376326772346c74737232736b3372",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        5,
        0,
    ],
    [
        None,
        None,
        None,
        "3c40246912b6efefab9a55244ac2c174e1a9f8c0bc0fd526933963c6ecb9b84ec8b0f6b40dc858fa23c72b",
        65530,
        None,
        "75317370757467353667736a763233637435346d7277646c616e7a7665716337747a73356d78786e616135636465676d303368673778363661797079647336356d39327674397561786c3637327375687063367a3768747776657079686b727066757376617a71756539",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        6,
        0,
    ],
    [
        None,
        "defa3d5a57efc2e1e9b01a035587d5fb1a38e01d",
        None,
        "cc099cc214e56b1192c7b5b17e958c3413e27fefd553380700aca81b24b2918cac951a1a68017fac525a18",
        65535,
        None,
        "75317667736b636d3939783567687561757668337978713777747037756e366130793663617964736e6e33357032647577707773356873367079676a6877703738326a716e65727a6c6878773370343971666d713237383339716a7472667976686b377964393877396e3064366a6e7336756834666333687364663736366b6e74716e6c6a646b64353667636e",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        7,
        0,
    ],
    [
        None,
        None,
        None,
        "5f09a9807a56323b263b05df368dc28391b21a64a0e1b40f9a6803b7e68f3905923f35cb01f119b223f493",
        65530,
        None,
        "75316378636379656d6d3038747964776d743968703273356e6638776a766c757575366c32653861396a666c6c647861736e7a6b6438667665727170636a30786e767261637a71673235356377356e767936783977727566666d703975657a727a72376763783535396b",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        8,
        0,
    ],
    [
        None,
        "10acd20b183e31d49f25c9a138f49b1a537edcf0",
        "9b60ae3d302248b349d601567e3d7795bfb334ea1fd1a7e71402169ebbe14bd2ceaa244ccd6e5aa2245613",
        "e340636542ece1c81285ed4eab448adbb5a8c0f4d386eeff337e88e6915f6c3ec1b6ea835a88d56612d2bd",
        65531,
        None,
        "75317a656b68686d686b353478356365356333367274376e63323735676570376e6176326e73783473683061666c6c75703976726835687338367a38736b6a746436646e736c7667736d6174743068386832343763676e666b73646c776c39786d617275797570666c743064716673637830647979656d3266616139776571653378616b397736656672353437636a3832397232746e7974613032687866647873646a6d76397a72356b746b70323066706378656164686672683032616b346136686e7876357336377267717272766670646a7435",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        9,
        0,
    ],
    [
        None,
        "af9db6990ed83dd64af3597c04323ea51b0052ad",
        None,
        "cdf7fed0d0822fd849cffb20a4d5ee701ad8141e66d81ddfabf87875117c05092240603c546b8dc187cd8c",
        65532,
        None,
        "753165353471636e30746570796c33307a7a326672677a37713461366d736e326530326e7076326e6666736433683532336d747838643232616a7666767371757235736a7a3876666e6d77327973363730387170386b6139306a3561343330757938763833616c6a63306330357a6a7535347879356e7677336d66686b376e7737366b6b7964796c713466656c",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        10,
        0,
    ],
    [
        None,
        None,
        None,
        "24fd59f32b2d39dde66e46c39206a31bc04fa5c6847976ea6bbd3163ee14f58f584acc131479ea558d3f84",
        65530,
        None,
        "75317a38777372686d66366d3967766136766c33737a636b303670393730783577686d36336a666a3266726d6d63396e39756d34796373387975746a37673833387672676832306c667879353279306832367474386e6776643267796370797176396b793032716b6373",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        11,
        0,
    ],
    [
        None,
        None,
        "78d85bd0db639043377987cdd814c6390016964b684016faf1ad4f166c5f72399a5e8d469ec6beb873d55d",
        None,
        65535,
        None,
        "75317861686a333570376d7639756c6b3337327333766465687172663438753077646633786c3772787a7270653461307468753864306d396d7961617078376b35767836747a357074636a76637675346472667137753771777a6d667565336b74387376736333736535",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        12,
        0,
    ],
    [
        "33a6dd87b4d872a4895d345761e4ec423b77928d",
        None,
        None,
        "5178924f7067eac261044ca27ba3cf52f798486973af0795e61587aa1b1ecad333dc520497edc61df88980",
        65533,
        "91e00c7a1d48af046827591e9733a97fa6b679f3dc601d008285edcbdae69ce8fc1be4aac00ff2711ebd931de518856878f73476f21a482ec9378365c8f7393c94e2885315eb4671098b79535e790fe53e29fef2b3766697ac32b4f473f468a008e72389fc03880d780cb07fcfaabe3f1a84b27db59a4a153d882d2b2103596555ed9494c6ac893c49723833ec8926c1",
        "7531687970706c733364776d616c783373756c746b72397564763237376679716a6478307378716c746638676a6e777976343968743575327270336c6c767632756e796d7330383675616a6b6638393837636175616a7136383670356638687276393474616336663078796637796d7a3636747279366b7936726179336d6a633567786661683030637370766b3564676d67736e3737663274336775763270307861366b6c6138717479376d6b6e6b6d337a68303932306c77733633326166743071686b3532363579736c337067323237747866373461736d7075656e326c746533616a6330667a376b34736878797a656d6e7035773770336b746c6874643030366d6b61787979306d746637646a73646175397a666b657332616e387661687a6737647173677938326330707830396d39683061657a736e7936786c66706767667268656d7661786a3578747871356a6e67763076306167726c3073757079676639636574656a35323779727a7a6574386471747164616771",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        13,
        0,
    ],
    [
        "a56c057ef71dab58aa90e47025695c5faaea5123",
        None,
        "a75a6de421d2ad1ee8f4b25e398adda9c0aaa6ab1f2518981a9ddb1de6a3957d77842332d6289dbe94e832",
        "b208c9235c8d40e49b76100b2d010f3783f12c66e7d3beb117b2c96321b7f6562adb4efc144e39d909e728",
        65533,
        None,
        "7531646670723876647335683361756e79657a7a7877726d38756461353273743837733876726c676732746730357430713070783336686368783974676b786b6c77747370753332786a6135617271336b7470326e387a613470773779776a30676d68713372776539353072386b3973756e736a76773734743538716c3333347065673464766b616c6b746d6e676e716b7077723332353837653779747932376e6d673636747371377976723779343639776570366b7077346a3530786e6c6d78306a78786737766c6735796c6671387566657664",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        14,
        0,
    ],
    [
        None,
        None,
        None,
        "9e5445d6cd3cb9f98b0df1062bda47adffd5a66c0c2c483c8bf15c3176d755914a3576496b5c35fee28a88",
        65531,
        None,
        "75316a676c686a326d617936646674777a39753271796e786a717a6e75743637343768617375306d646d6c63303266636173756178756764797a776a326c38346d6a3966677a6a3779306b396663706a373336736c6d6a38676b37377567386c6c61766367326c666d6d",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        15,
        0,
    ],
    [
        "b02aec10f6fa02a08667bf9b924c3d0574a1334f",
        None,
        None,
        "2598d84dffb34f5908b90732490f3881399150d4c694fce9bf30d1560b2c56f09829fe123b9add20e5d71c",
        65534,
        None,
        "7531397163617a647761793438707566366a77616a78307732386d307871756d746d6e6435677974796c6c6e79676867396c76393978356d3872387439673566396a307a30786e34787a6d6e7866747a3772746633756164786b79367178706e6b7438666b66686c78386b63396d6e72646c6e7874733536786378656a7a6472776c65787a7637377876797634",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        16,
        0,
    ],
    [
        None,
        None,
        "d3a803803feee7a032a24adfaa8f6a94cecb9671c1333d0d5d1a3d79d82bc310727c665364d71022559c50",
        "7c98b8f613f9ff02746bea2a167cfd1bd3a1862af9631bf61d9d604e0824e2cb8467a1e549db87a76e7a8a",
        65535,
        None,
        "75316136346c303971727378756c666a7a6e6d366b326735333575737968746166386564363076346a726a6d6b77766b757834743770647963336e6b7a7265666467746e77383432306c6a3873686d30356a6139667878676e68726139326e6873713536677838633270757a33666b6b676e726b7166357975716664746637743672616e343767646366357676646661637a7766337575793466797368336d7a7538686435746b6c30356d76726765396e38",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        17,
        0,
    ],
    [
        "26c061d67beb8bad48c6b4774a156551e30e4fe2",
        None,
        None,
        "a80405d5568ab8ab8f8546163d951ab297fd5e6f43e7fcebcb664feacfab5afd80aaf7f354c07a9901788c",
        65535,
        None,
        "7531787a757764386163686667776d336577793976326d6a3537373268726b6e6d6578777a6339346d7a6133356d78363863656e767877727a3973396670306e39767a753872756a357a71666d6d376c65387775366c363275346c6d30376e75717865656d383733677838366a766e776c70787379636c397576366b786b72686d30726c677037307830357366",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        18,
        0,
    ],
    [
        None,
        None,
        "8660070e3757ff6507060791fd694f6a631b8495a2b74ffa39236cf653caea5575b86af3200b010e513bab",
        "63b7b706d991169986aee56133f0a50b2a0c8225fba6dae95176007b1f023a1e97c1aa366e99bf970fda82",
        65534,
        None,
        "7531766736326d676a64646e6c763577366c646b793278653063387465746d633832747539766c7a7a6b75796e783439666e75716a76786a743564676e33636d3874356e38357a6371356c6a727467377a6d77686b3730683672646d636c6637736378786e67756b35666c76663261707037367875393037636d6a796c787673656e3235786539763776336b727378613975793076326a6a7133376b6834796d6c61666e3870657671616c716134646d3637",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        19,
        5,
    ],
]


class ZcashTestVector:
    def __init__(self, inner):
        self.inner = inner

    def __getattr__(self, name):
        index = TESTVECTORS[1][0].split(", ").index(name)
        return self.inner[index]


def get_receivers(tv: ZcashTestVector):
    receivers = dict()
    if tv.p2pkh_bytes is not None:
        receivers[P2PKH] = unhexlify(tv.p2pkh_bytes)
    if tv.p2sh_bytes is not None:
        receivers[P2SH] = unhexlify(tv.p2sh_bytes)
    if tv.sapling_raw_addr is not None:
        receivers[SAPLING] = unhexlify(tv.sapling_raw_addr)
    if tv.orchard_raw_addr is not None:
        receivers[ORCHARD] = unhexlify(tv.orchard_raw_addr)
    if tv.unknown_bytes is not None:
        receivers[tv.unknown_typecode] = unhexlify(tv.unknown_bytes)

    return receivers


COIN = coininfo.by_name("Zcash")


@unittest.skipUnless(not utils.BITCOIN_ONLY, "altcoin")
class TestZcashAddress(unittest.TestCase):
    def test_encode_unified(self):
        for tv in map(ZcashTestVector, TESTVECTORS[2:]):
            receivers = get_receivers(tv)
            ua = unified_addresses.encode(receivers, COIN)
            self.assertEqual(ua, unhexlify(tv.unified_addr).decode())

    def test_decode_unified(self):
        for tv in map(ZcashTestVector, TESTVECTORS[2:]):
            address = unhexlify(tv.unified_addr).decode()
            receivers = unified_addresses.decode(address, COIN)
            self.assertEqual(receivers, get_receivers(tv))


if __name__ == "__main__":
    unittest.main()
