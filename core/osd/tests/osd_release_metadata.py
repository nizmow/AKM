"""Structural metadata captured from the pre-retirement Release OSD files.

These values are report-only. Runtime probes and source-archive parity checks
remain the OSD smoke tests' pass/fail contracts.
"""

RELEASES = {
    "OFF002": {
        "version": "0411", "length": 12440, "tlen": 0x3010, "dlen": 0x20,
        "organisation": (2, 1, 2, 1),
        "entries": (0x52, 0x68, 0x140, 0x1F6, 0x6A, 0xAC, 0xFA, 0x13E),
        "sha256": "4dc9b08c229d852fe2587dc0b65f6b1a5e692ac463427f296256212f9e74df04",
    },
    "OFF004IP": {
        "version": "0411", "length": 14013, "tlen": 0x3656, "dlen": 0x8,
        "organisation": (4, 2, 0, 1),
        "entries": (0x52, 0x60, 0x146, 0x1DC, 0x62, 0xA4, 0xF2, 0x144),
        "sha256": "d79d0c69b03d459f624f35fbaf62d6526db8f53c6ba02879328aa1bc1a70da88",
    },
    "OFF016IP": {
        "version": "0410", "length": 15666, "tlen": 0x3CB0, "dlen": 0x20,
        "organisation": (16, 4, 0, 1),
        "entries": (0x52, 0x60, 0x146, 0x1DC, 0x62, 0xA4, 0xF2, 0x144),
        "sha256": "c29fdfe6b4da37d6bd84ddff4dfc0210c090c92a1db7baf6877eeceff2739082",
    },
    "OFF256IP": {
        "version": "0410", "length": 22558, "tlen": 0x5798, "dlen": 0x1C,
        "organisation": (256, 8, 0, 1),
        "entries": (0x52, 0x70, 0x1A2, 0x238, 0x72, 0xB4, 0x102, 0x14A),
        "sha256": "76fff6ddd742ae8dec4d52b1dbc57ea3244ea75037ad27d346d13e2b85f8cd05",
    },
    "OFF016PK": {
        "version": "0411", "length": 14866, "tlen": 0x38CE, "dlen": 0xD0,
        "organisation": (16, 4, 2, 1),
        "entries": (0x52, 0x74, 0x18E, 0x224, 0x76, 0xB8, 0x106, 0x158),
        "sha256": "50f84ecdfc1d7abff901b30480a4602dbdb05da3fc366ec151607a1a65b199e4",
    },
    "OFF16MOV": {
        "version": "0413", "length": 8663, "tlen": 0x1BB8, "dlen": 0x5E0,
        "organisation": (16777216, 32, 2, 129),
        "entries": (0x52, 0x7A, 0x226, 0x2FE, 0x7C, 0xBC, 0x10A, 0),
        "sha256": "893f1a452a954c6412371462bbca7cf0cff2a345db7c95e748855a3937f29afa",
    },
    "OFF256PK": {
        "version": "0412", "length": 18171, "tlen": 0x4692, "dlen": 0,
        "organisation": (256, 8, 2, 1),
        "entries": (0x52, 0x8A, 0x1D2, 0x268, 0x8C, 0xCE, 0x11C, 0x164),
        "sha256": "40ac561d487a0dd836d9fd470286ad6f5ec2cfe093f8afa696de6fe7d9f1cd04",
    },
    "OFF32KPK": {
        "version": "0413", "length": 9170, "tlen": 0x1F66, "dlen": 0x3E0,
        "organisation": (32768, 16, 2, 129),
        "entries": (0x52, 0x7A, 0x226, 0x2EE, 0x7C, 0xBC, 0x10A, 0),
        "sha256": "4f052c06318d2db187ec153aa72578b3d77ead7aa403b52259338303d7433262",
    },
    "OFF65KPK": {
        "version": "0413", "length": 9166, "tlen": 0x1F62, "dlen": 0x3E0,
        "organisation": (65536, 16, 2, 129),
        "entries": (0x52, 0x7A, 0x226, 0x2EE, 0x7C, 0xBC, 0x10A, 0),
        "sha256": "817ecfe9139a67f5d3ee4d4d419bbeb96112d9d268644fc9c399076bf54a04e7",
    },
}


ENTRY_NAMES = (
    "init", "reset", "wk_init", "wk_reset", "opnwkinfo", "extndinfo",
    "scrninfo", "resinfo",
)


def release_path(name: str) -> str:
    return f"pre-magic-retirement:MagiC/Releases/MAGIC.A/GEMSYS/{name}.OSD"


def relocation_length(metadata: dict) -> int:
    return metadata["length"] - (0x1C + metadata["tlen"] + metadata["dlen"])


def release_metadata(name: str) -> dict:
    metadata = RELEASES[name].copy()
    metadata["entries"] = dict(zip(ENTRY_NAMES, metadata["entries"]))
    metadata["reloc_len"] = relocation_length(metadata)
    metadata["path"] = release_path(name)
    return metadata
