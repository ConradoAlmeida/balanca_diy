# Projeto mecânico — SolidWorks

O projeto mecânico da balança é a **fonte da verdade** e fica no NAS, no share
`documentos`. O firmware e a documentação de hardware ficam neste repositório;
os binários do CAD **não são versionados aqui**.

## Fonte

```text
\\192.168.0.150\documentos\Projetos_3D\balanca_diy
```

No Linux (montagem gvfs):

```text
/run/user/1000/gvfs/smb-share:server=homelab.local,share=documentos/Projetos_3D/balanca_diy
```

## Arquivos

| Arquivo | Tipo | Papel |
|---|---|---|
| `balanca_conrado.SLDPRT` | SolidWorks | **Fonte principal** (~1,7 MB) |
| `balanca_conrado_base_v0.STEP` | STEP | Base |
| `balanca_conrado_pes_v0.STEP` | STEP | Pés |
| `balanca_conrado_tampa_v0.STEP` | STEP | Tampa |
| `balanca_conrado_tampa_display_pes_v0.STEP` | STEP | Tampa com display e pés |

Total: ~3,9 MB.

## Como usar

- **Editar:** abra o `balanca_conrado.SLDPRT` no SolidWorks (VM Windows) e salve
  de volta no NAS.
- **Imprimir/visualizar:** importe um dos `.STEP` no slicer, FreeCAD ou outro
  CAD. Os STEPs são exports de intercâmbio; o `.SLDPRT` é a fonte.
- **Conferir o hardware:** componentes, pinagem e dimensões estão em
  [HARDWARE.md](HARDWARE.md); bateria e autonomia em [BATERIA.md](BATERIA.md).

## Versionamento

Os arquivos `.SLDPRT`/`.STEP` **não entram no Git** (binários e fonte no NAS).
Se for necessário publicar uma versão, prefira anexar o STEP a um release em vez
de commitar no repositório.
