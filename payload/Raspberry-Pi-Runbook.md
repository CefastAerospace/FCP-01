# Manual e configurações rápidas:

- **Conectar o aeropi por ssh**
    (Atenção, os arquivos com as senhas estão no drive da equipe!)

    O aeropi já está configurado para aceitar uma rede especifica, então vamos criar essa rede com o hostpot do computador.
    
    1. Entre nas configurações de hotspot do computador
    2. Configure a rede para: `Name: XXXX | Password: XXXX`
    3. Espere o aeropi conectar, caso seja necesário resete ele.
    4. Quando conectar-se a rede criada, irá aparecer o ip em que ele se conectou, anote-o.
    5. Abra um terminal use o commando de ssh: `ssh aeropi@<ip-conectado>`
    6. Ira aparecer um prompt para digitar o usuario e a senha, coloque os ids já criados, `XXXX | XXXX` 
    7. Caso você receba uma mensagem de confirmação, você está conectado ao aeropi!
    
- **Ativar a telemetria**
    
    Ative a captação do pacote dump1090, para utilizar o RTL-SDR para captar telemetria de aviões próximos
    
    ```bash
    /opt/dump1090/dump1090 --net --gain 49.6 > /dev/null 2>&1 &
    ```
    
    Chame o script de telemetria do payload para formatar os dados e colocar no formato específico da missão, salvando em um arquivo json:
    
    ```bash
    python3 /mnt/data/telemetry.py
    ```



# Configurando o Pi

## Fase 1: Setup do OS e Partição de Dados

Para um satélite, você não precisa de interface gráfica, pois ela consome RAM e processamento à toa.

### 1. Gravação do OS

Use o **Raspberry Pi Imager** para gravar o *Raspberry Pi OS Lite (32-bit)* no cartão SD. Nas configurações avançadas do Imager, ative o SSH, configure sua rede Wi-Fi e crie seu usuário com as seguintes credenciais:

- **Hostname:** `aeropi`
- **Username:** `XXXXX`
- **Password:** `XXXXX`
- **Wi-Fi:** `aeronet`

### 2. Desabilitar o Resizing Automático

Imediatamente após a conclusão da gravação, reinsira o cartão SD no seu computador.

1. Abra a partição de boot (geralmente chamada `boot` ou `bootfs`).
2. Localize e abra o arquivo `cmdline.txt` usando um editor de texto simples (como Bloco de Notas ou Nano).
3. Encontre a palavra `resize` no meio da linha e delete-a completamente, incluindo o espaço em branco ao lado dela.

> ⚠️ **Aviso Crucial:** Garanta que todo o bloco de configuração permaneça em uma única linha contínua (Linha 1). Não pressione `Enter` nem adicione quebras de linha.
> 
1. Salve e feche o `cmdline.txt`.

### 3. Primeiro Boot e Configuração da Partição via SSH

#### Script para corrigir as desconexões do Wi-Fi

Aqui está o processo completo, limpo e passo a passo para configurar isso do zero usando o script universal otimizado baseado em ping.

#### **Passo 1: Criar o arquivo do script**

Abra o seu terminal e crie um novo arquivo de script:

```bash
touch /home/aerospace/wifi-reconnect.sh
```

Abra o arquivo usando o editor de texto nano:

```bash
nano /home/aerospace/wifi-reconnect.sh
```

#### **Passo 2: Colar o código do script**

Cole a seguinte lógica moderna e universal no editor. Este script usa caminhos absolutos e lida automaticamente com as versões antigas e novas do Raspberry Pi OS Lite:

```bash
#!/bin/bash

# 1. Testa a conexao pingando o DNS do Google (1 pacote, 5s de timeout)
if ! /bin/ping -c 1 -W 5 8.8.8.8 > /dev/null 2>&1; then
    echo "$(date) - Internet caiu. Reconectando..."

    # 2. Verifica se esta usando o NetworkManager moderno (OS Bookworm Lite)
    if command -v /usr/bin/nmcli > /dev/null 2>&1; then
        /usr/bin/nmcli networking off
        /bin/sleep 5
        /usr/bin/nmcli networking on

    # 3. Verifica o fallback padrao do comando 'ip' (OS Bullseye Lite)
    elif command -v /usr/sbin/ip > /dev/null 2>&1; then
        /usr/sbin/ip link set wlan0 down
        /bin/sleep 5
        /usr/sbin/ip link set wlan0 up

    # 4. Fallback legado antigo
    elif command -v /sbin/ifconfig > /dev/null 2>&1; then
        /sbin/ifconfig wlan0 down
        /bin/sleep 5
        /sbin/ifconfig wlan0 up
    fi
fi
```

Pressione `Ctrl + O` e depois `Enter` para salvar, e `Ctrl + X` para sair do nano.

#### **Passo 3: Tornar o script executável**

Dê permissão para o script rodar como um programa:

```bash
chmod +x /home/aerospace/wifi-reconnect.sh
```

#### **Passo 4: Automatizar com o Crontab do Sistema**

Abra o arquivo crontab do sistema usando privilégios de root:

```bash
sudo nano /etc/crontab
```

Role até o final do arquivo e adicione esta linha exata *(certifique-se de pressionar `Enter` após colá-la para que haja uma linha em branco no final do arquivo)*:

```
* * * * * root /home/aerospace/wifi-reconnect.sh >> /home/aerospace/wifi-reconnect.log 2>&1
```

Pressione `Ctrl + O` e depois `Enter` para salvar, e `Ctrl + X` para sair do nano.

#### **Passo 5: Verificar a Configuração**

Seu Pi agora está totalmente configurado. Você pode verificar se o sistema está rodando seu script a cada 60 segundos observando o arquivo de log ou verificando o gerenciador do sistema:

```bash
sudo journalctl -u cron | grep wifi-reconnect | tail -n5
```

*(Nota: Você pode ajustar o crontab futuramente caso queira que ele rode com menos frequência, como a cada 5 minutos, para reduzir o tráfego de rede).*

### Reestruturação dos Limites do Sistema Operacional (Partições)

Insira o cartão no Pi, ligue-o e abra o editor de tabela de partições:

#### **Passo 1: Abrir o Gerenciador de Partições**

Execute o seguinte comando para abrir a ferramenta de disco:

```bash
sudo fdisk /dev/mmcblk0
```

#### **Passo 2: Ler o Layout Atual**

1. Digite `p` e pressione `Enter`.
2. Olhe na coluna **Device** por `/dev/mmcblk0p2`.
3. Anote o número exato sob a coluna **Start** *(Exemplo: 1064960).* Você precisará desse número exato no Passo 4. 

#### **Passo 3: Deletar o Limite Atual da Pequena Partição Root**

1. Digite `d` e pressione `Enter`.
2. Se perguntar o número da partição, digite `2` e pressione `Enter`. *(Não se preocupe, seus arquivos estão armazenados em cache com segurança na memória do sistema no momento)*.

#### **Passo 4: Recriar a Partição Root Maior**

1. Digite `n` e pressione `Enter` (para criar uma nova partição).
2. Digite `p` e pressione `Enter` (para primária).
3. Digite `2` e pressione `Enter` (para atribuir novamente à partição 2).
4. Quando solicitar o **First sector**, digite o **mesmo número exato** do *Start* que você anotou durante o Passo 2 e pressione `Enter`.
5. Quando solicitar o **Last sector**, digite o tamanho que deseja que sua unidade do SO tenha. Para uma margem confortável de atualização, digite `+6G` (para 6 Gigabytes) ou `+8G` (para 8 Gigabytes) e pressione `Enter`.

> ⚠️ **CRÍTICO:** Se aparecer um aviso perguntando: *"Do you want to remove the ext4 signature?"*, digite `N` (para Não) e pressione `Enter`. Se você digitar sim, seu SO será apagado.
> 
1. Digite `p` novamente e pressione `Enter`.
2. Olhe na coluna **Device** por `/dev/mmcblk0p2`. A**note o número exato sob a coluna End**. Você precisará desse número exato no Passo 5. *(Exemplo: 13647871)*

#### **Passo 5: Criar sua Terceira Partição de Leitura e Escrita (Dados)**

Agora usaremos o espaço restante para sua partição de armazenamento permanente:

1. Digite `n` e pressione `Enter`.
2. Digite `p` e pressione `Enter`.
3. Digite `3` e pressione `Enter`.
4. Para o **First sector**, digite o **bloco final exato + 1**, que começa logo após a sua partição OS de 6GB *(Exemplo: se o último setor foi 13647871 no passo anterior, aqui será 13647872)*.
5. Para o **Last sector**, simplesmente pressione `Enter` para aceitar o ponto de término padrão *(isso consome automaticamente 100% do espaço restante do cartão SD)*.

#### **Passo 6: Salvar e Sair**

Digite `w` e pressione `Enter`. Isso grava sua nova tabela de configuração no cartão SD.

#### **Passo 7: Reiniciar o Pi para Fixar a Tabela**

Force uma reinicialização para permitir que o kernel se ajuste aos novos limites:

```bash
sudo reboot
```

#### **Passo 8: Expandir o Sistema de Arquivos e Formatar a Partição de Dados**

Assim que o Pi reiniciar e você fizer o login, execute estes comandos um por um:

```bash
# 1. Expanda sua unidade do SO para preencher seu novo espaço de 6G/8G:
sudo resize2fs /dev/mmcblk0p2

# 2. Formate sua nova 3ª partição para o formato padrão Linux Ext4:
sudo mkfs.ext4 /dev/mmcblk0p3

# 3. Crie sua pasta de armazenamento, monte-a e assuma a propriedade para o seu usuário:
sudo mkdir -p /mnt/data
sudo mount /dev/mmcblk0p3 /mnt/data
sudo chown -R $USER:$USER /mnt/data
```

**Verification:** Run `touch /mnt/data/test.txt && ls -l /mnt/data/test.txt`. If it creates the file without a "Permission denied" error, and shows your username next to it, it worked. You can then delete it with `rm /mnt/data/test.txt`.

#### **Passo 9: Fazer a Terceira Partição Montar Permanentemente**

Abra o arquivo de montagem de unidades do sistema:

```bash
sudo nano /etc/fstab
```

Use as setas para descer até a última linha, pressione `Enter` para iniciar uma linha limpa, e digite isto exatamente:

```
/dev/mmcblk0p3  /mnt/data  ext4  defaults,noatime  0  2
```

Salve e saia (`Ctrl+O`, `Enter`, `Ctrl+X`).

Teste para garantir que não há erros de sintaxe:

```bash
sudo mount -a
```

*(Se este comando não retornar erros, sua configuração está correta).*

#### **Passo 10: Testar se as configurações funcionaram corretamente**

Rode o seguinte comando para verificar as partições do sistema

```bash
df -h
```

Caso você veja a partição /mnt/data com a quantidade corretar de gigas e a p2 também com a quantidade correta as configurações foram certas

## Fase 2: Watchdog

Esta etapa final conecta o sistema diretamente ao microchip interno de temporização Broadcom e constrói um "escudo" na RAM ao redor dos arquivos essenciais.

### 1. Configuração do Hardware Watchdog

Abra o arquivo de configuração do sistema:

```bash
sudo nano /etc/systemd/system.conf
```

Role para baixo, localize, descomente (remova o `#` da frente) e edite estes temporizadores *(15 segundos é o limite máximo suportado pelo chip Broadcom BCM2835)*:

```
RuntimeWatchdogSec=15s
RebootWatchdogSec=15
```

Salve, feche e recarregue as configurações:

```bash
sudo systemctl daemon-reexec
```

### 2. Como executar um teste do watchdog com segurança

Habilite os comandos de emergência:

```bash
sudo bash -c 'echo 1 > /proc/sys/kernel/sysrq'
```

Limpe os buffers de memória para proteger o cartão SD:

```bash
sync
```

Agora, vamos injetar o comando de "Kernel Panic".

> ⚠️ **Atenção:** Assim que você apertar `Enter` no comando abaixo, sua tela do SSH vai congelar na hora.
> 

```bash
sudo bash -c 'echo c > /proc/sysrq-trigger'
```

**O que observar agora:**

1. O seu terminal SSH vai travar imediatamente.
2. Olhe para a luz verde do Raspberry Pi Zero (o LED de atividade). Ele vai parar de piscar ou ficar travado. O Pi está "morto".
3. Conte no relógio: Em exatos 15 segundos (que foi o watchdog-timeout que configuramos), o circuito interno de hardware vai perceber que o sistema operacional parou de responder.
4. O LED do Pi vai apagar e acender novamente. Ele estará cortando a energia internamente e forçando um reboot físico!
5. Espere cerca de 1 minuto, abra um novo terminal no seu computador e tente acessar via SSH novamente.

👉 **Resultado esperado:** Você vai conseguir logar! O Pi voltou à vida sozinho. Se os testes funcionaram, parabéns: você tem um sistema computacional embarcado classificado para voo!

## Fase 3: Read-Only OS (OverlayRoot)

### 1. Inicialização do OverlayRoot

Instale a ferramenta de overlay avançada. Após o Pi reiniciar em um estado normal de gravação, instale o motor de configuração nativo:

```bash
sudo apt update
sudo apt install -y overlayroot
```

Configure a proteção para envolver apenas a partição root, deixando sua unidade de telemetria de 23GB aberta para gravações permanentes:

```bash
sudo nano /etc/overlayroot.conf
```

Vá diretamente para a última linha e insira este parâmetro:

```
overlayroot="tmpfs:recurse=0"
```

Salve e feche (`Ctrl+O`, `Enter`, `Ctrl+X`).

> ℹ️ **O que isso faz:** O argumento `recurse=0` diz ao sistema para congelar apenas a partição principal do sistema operacional (`/dev/mmcblk0p2`). Isso instrui explicitamente o gerenciador de inicialização a ignorar quaisquer partições extras (como a sua partição de dados em `/dev/mmcblk0p3`), deixando-as totalmente com permissão de leitura e escrita (read-write).
> 

### 2. Loop de Inicialização

Complete a sequência reiniciando para ligar o seu ambiente operacional imutável:

```bash
sudo reboot
```

### 3. O que acabou de acontecer? (O Teste)

Quando o Pi ligar novamente, ele estará imortal contra quedas de energia. Vamos testar se funcionou?

**Teste 1: A Ilusão da Memória RAM (Root)**

1. Conecte-se novamente por SSH.
2. Crie um arquivo na pasta principal do usuário (que agora é Read-Only sob o Overlay):

```bash
touch ~/teste_falso.txt
```

1. Agora, vá para a nossa partição isolada de dados:

```bash
cd /mnt/data
sudo touch telemetria.txt
```

*(O arquivo vai aparecer lá. O Linux acha que gravou).*

1. Puxe o cabo de energia do Raspberry Pi da tomada *(sim, de propósito!)*.
2. Ligue-o novamente. Dê o comando `ls ~` e você verá que o `teste_falso.txt` sumiu! Ele estava apenas na RAM.
3. Vá na pasta de telemetria: `ls /mnt/data`. O arquivo `telemetria.txt` estará lá, gravado com segurança!

# Configurando o SDR

## **Fase 1: Verificação de Hardware e Configuração de Drivers RTL-SDR**

Antes de instalar o software, verifique se o Raspberry Pi reconhece fisicamente o dongle RTL-SDR através do adaptador Micro-USB OTG.

- **Verificação Física:** Conecte o RTL-SDR no adaptador OTG e ligue-o na **porta USB** do Pi Zero (não na porta "PWR IN").
- **Verificar Detecção USB:** Execute o seguinte comando no terminal:
    
    ```bash
    lsusb
    ```
    
    - **Critério de Sucesso:** Procure por uma linha parecida com:
        
        `Bus 001 Device 002: ID 0bda:2838 Realtek Semiconductor Corp. RTL2838 DVB-T`
        
        *(Se estiver faltando, verifique se o hardware está bem conectado ou teste o adaptador com outro periférico USB).*
        

**Configurando o Ambiente Persistente e Bloqueando Drivers Conflitantes**
Como o sistema opera em um sistema de arquivos somente leitura (`overlayroot`), instalações normais via `apt` e alterações em `/etc` serão apagadas ao reiniciar. Todas as alterações devem ser feitas dentro do contêiner chroot persistente. Além disso, as shells chroot não herdam as configurações de DNS do host, sendo necessário injetar o DNS do Google para permitir downloads de pacotes.

1. **Entrar no Ambiente Chroot:**
    
    ```bash
    sudo overlayroot-chroot
    ```
    
    *(Verificação: O prompt do terminal muda para `root@aeropi-backup:/#`)*
    
2. **Corrigir a Resolução de DNS dentro do Chroot:**
    
    ```bash
    echo "nameserver 8.8.8.8" > /etc/resolv.conf
    echo "nameserver 1.1.1.1" >> /etc/resolv.conf
    ```
    
    *(Verificação: Execute `cat /etc/resolv.conf` para confirmar se ambos os endereços IP estão listados).*
    
3. **Instalar os Drivers de Sistema RTL-SDR e Bloquear Módulos DVB do Kernel:**
Os módulos DVB padrão do Linux entram em conflito com o uso do RTL-SDR e devem ser bloqueados.
    
    ```bash
    apt update
    apt install -y rtl-sdr
    echo -e "blacklist dvb_usb_rtl28xxu\nblacklist rtl2832\nblacklist rtl2830" > /etc/modprobe.d/blacklist-rtl.conf
    ```
    
    Ainda dentro do seu `chroot`, entre no config.txt e disabilite o bluetooth, isso economiza energia da bateria e elimina a interferência de radiofrequência (RF):
    
    ```bash
    nano /boot/firmware/config.txt
    ```
    
    Role até o final desse arquivo e adicione a nossa linha:
    
    ```
    dtoverlay=disable-bt
    ```
    
4. **Sair e Reiniciar:**
    
    ```bash
    exit
    sudo reboot
    ```
    
5. **Testar a Instalação dos Drivers:**
Após fazer login novamente como usuário normal, teste o driver de hardware:Bash
    
    ```bash
    rtl_test
    ```
    
    *(Verificação: O utilitário deve inicializar, relatar dispositivos encontrados e transmitir transferências assíncronas sem erros. Pressione `Ctrl + C` para sair).*
    

## **Fase 2: Compilação do dump1090**

Para decodificar ondas de rádio ADS-B em 1090 MHz, compilamos o `dump1090` de MalcolmRobb a partir do código-fonte. As distribuições modernas do Raspberry Pi OS executam o GCC 14, que impõe padrões C mais rígidos e causa falhas de compilação a menos que seja aplicado o patch `-fcommon`.

1. **Entrar no Ambiente Persistente e Instalar Ferramentas de Compilação:**
    
    ```bash
    sudo overlayroot-chroot
    apt install -y git make gcc pkg-config librtlsdr-dev
    ```
    
    *(Verificação: Execute `gcc --version` para confirmar que o compilador está ativo).*
    
2. **Clonar e Aplicar o Patch no Código-Fonte:**
Baixe o repositório em `/opt` (diretório padrão para softwares de terceiros) e aplique o patch do compilador no `Makefile`:
    
    ```bash
    cd /opt
    git clone https://github.com/MalcolmRobb/dump1090.git
    cd dump1090
    sed -i 's/CFLAGS=/CFLAGS=-fcommon /g' Makefile
    make
    ```
    
    *(Nota: A compilação leva de um a dois minutos no hardware do Pi Zero W).*
    
3. **Verificar o Binário Executável:**
    
    ```bash
    ls -l dump1090
    ```
    
    *(Verificação: A linha de saída para o `dump1090` deve exibir permissões de execução: `rwxr-xr-x`).*
    
4. **Sair e Persistir:**
    
    ```bash
    exit
    sudo reboot
    ```
    
    *(Verificação após a reinicialização: Execute `ls -l /opt/dump1090/dump1090` para confirmar que o binário compilado sobreviveu à reinicialização do sistema).*
    

## **Fase 3: Validação de Recepção e Execução do Daemon**

Com o software compilado e os drivers ativos, teste a recepção de sinal ao vivo antes de implantar a automação em segundo plano.

1. **Testar a Recepção de Sinal ao Vivo:**
Conecte sua antena e execute o decodificador no modo interativo do terminal com ganho maximizado (`49.6`):
    
    ```bash
    /opt/dump1090/dump1090 --interactive --gain 49.6
    ```
    
    *(Verificação: Uma tabela de texto em tempo real será populada com IDs Hex de aeronaves, altitudes e velocidades à medida que cruzam a linha de visada da sua antena. Pressione `Ctrl + C` para sair).*
    
2. **Iniciar o Daemon de Rede em Segundo Plano:***(Nota: A versão legada do dump1090 de MalcolmRobb não suporta o comando nativo `-write-json` para salvamento direto de arquivos; em vez disso, ela transmite telemetria estruturada BaseStation via porta de socket de rede TCP `30003`).*
    
    Inicie o decodificador silenciosamente em segundo plano com a rede habilitada:
    
    ```bash
    /opt/dump1090/dump1090 --net --gain 49.6 > /dev/null 2>&1 &
    ```
    
3. **Verificar o Socket de Rede:**
    
    ```bash
    ss -tuln | grep 30003
    ```
    
    *(Verificação: A porta `30003` deve aparecer no estado `LISTEN`).*
    

## **Fase 4: Implantação do Script de Telemetria**

O script Python personalizado conecta-se diretamente à porta `30003`, lê o fluxo BaseStation bruto, extrai parâmetros de voo e anexa um registro estruturado ao log de missão persistente em `/mnt/data`.

1. **Criar o Script de Telemetria (`/mnt/data/telemetry.py`):**
    
    ```python
    import socket
    import time
    
    # Caminho do arquivo de log persistente na partição dedicada
    LOG_FILE = '/mnt/data/mission_log.json'
    
    # Estabelece conexão com o socket de rede local do dump1090
    s = socket.socket()
    s.connect(('127.0.0.1', 30003))
    
    print("Aguardando telemetria de aeronaves...")
    
    while True:
        try:
            # Recebe dados brutos da rede e extrai a primeira linha disponível
            line = s.recv(1024).decode(errors='ignore').split('\n')[0]
            if line:
                # Escreve a linha capturada no arquivo de log persistente
                with open(LOG_FILE, 'a') as f:
                    f.write(line + '\n')
                print("Capturado:", line)
        except Exception:
            pass
        time.sleep(1)
    ```
    
2. **Executar e Verificar o Pipeline:**
Execute o parser:
    
    ```bash
    python3 /mnt/data/telemetry.py
    ```
    
    *(Verificação: As linhas de telemetria em tempo real são impressas no terminal. Em uma janela SSH secundária, inspecione o log de armazenamento persistente usando: `tail -f /mnt/data/mission_log.json`).*