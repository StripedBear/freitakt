# FREITAKT: the text of the Russian user manual (HTML fragments), built by tools/make_manual_html.py.
# Markers: {{PANEL}} the panel schematic; {{shot:FILE|caption}} a screenshot; {{screen:FILE|caption}} ... {{/screen}}
# a screenshot with its description at the right. FILE is relative to build/ui_shots.
# Every combo here was checked against firmware/src at f148566 (ui_input.c, ui.c, ui_tools.c, cue.c, seq.c, ...).

CONTENT = r'''
<h2 id="intro">1. Коротко о FREITAKT</h2>

<p><b>FREITAKT</b> — облегчённая версия прошивки Hortator/Felucca для грувбокса <b>M-VAVE FM-1</b>.
Она превращает FM-1 в драм-машину в духе Elektron Model:Cycles: <b>6 треков</b>, у каждого своя
<b>машина</b> (модель звука), свои фильтр, инсерт-эффект, посылы, LFO и арпеджиатор/ретриггер. Секвенсор хранит
<b>4 паттерна (A–D) × 8 вариаций</b>. У каждого трека своя длина (до 64 шагов) и свой шаг сетки, включая
FIT-деления для полиритмов. Ещё есть общая длина Master Length, цепочки паттернов,
<b>parameter lock</b>-и и <b>machine lock</b>-и на шагах, live-запись, MIDI OUT, режим MIDI-клавиатуры и Bluetooth MIDI.</p>

<p>Названия «форм» в этом руководстве взяты из Model:Cycles: <b>паттерн</b> (A–D), <b>вариация</b> (1–8),
<b>трек</b> (T1–T6), <b>шаг</b>, <b>parameter lock</b> (значение ручки на одном шаге),
<b>machine lock</b> (другая машина на одном шаге). Надписи экрана приводятся как есть, моноширинным шрифтом:
<code>LEN</code>, <code>CLEAR T1</code>.</p>

<div class="note"><b>Обозначения.</b> <kbd>SEQ</kbd> — кнопка, <kbd class="k">KNOB 1</kbd> — ручка или клавиша.
«<kbd>SAVE</kbd> + <kbd>REC</kbd>» значит «держать SAVE и нажать REC».
«Белая 1–16» — белые клавиши слева направо (F3 … G5), «чёрная 1–11» — чёрные слева направо (F#3 … F#5).
«Тап» — короткое нажатие, «удержание» — нажать и держать (время указано, где оно важно).</div>

<h3 id="panel">Карта панели</h3>
{{PANEL}}
<p>Схема упрощена: расположение примерное, надписи как на панели FM-1.</p>

<table>
<thead><tr><th>Орган</th><th>Что делает</th></tr></thead>
<tbody>
<tr><td><kbd class="k">MASTER</kbd></td><td>Громкость (динамик, наушники). Аналоговая ручка, прошивка её не читает.</td></tr>
<tr><td><kbd class="k">SELECT</kbd> (энкодер)</td><td>Листает страницы текущего раздела вперёд/назад, на краях останавливается.
На TRACKS задаёт темп. В меню, в карусели SAVE и в списке пресетов прокручивает пункты.</td></tr>
<tr><td><kbd class="k">ALGORITHM</kbd> (энкодер)</td><td>Выбирает трек T1–T6 на любой странице.</td></tr>
<tr><td><kbd class="k">PRESETS</kbd> (энкодер)</td><td>Каждый щелчок загружает в выбранный трек следующий или предыдущий пресет; вместо страницы на весь экран появляется список пресетов, страница возвращается через 3 с после последнего щелчка.</td></tr>
<tr><td><kbd class="k">KNOB 1–4</kbd></td><td>Четыре строки текущей страницы, сверху вниз.</td></tr>
<tr><td><kbd>HOME</kbd></td><td>Тап: экран HOME ⇄ TRACKS (с любой другой страницы — тот из них, что был последним). Удержание 0,7 с: меню устройства.</td></tr>
<tr><td><kbd>EDIT</kbd></td><td>Звук выбранного трека: страницы SOUND, затем ARP. Срабатывает на нажатии. Удержание 0,4 с: список пресетов.</td></tr>
<tr><td><kbd>FX</kbd></td><td>Тап: страницы фильтра, посылов, инсерта и компрессора. Удержание 0,4 с: PERFORM.</td></tr>
<tr><td><kbd>LFO</kbd></td><td>Страницы LFO и его назначений (DEST).</td></tr>
<tr><td><kbd>SEQ</kbd></td><td>Страница PATTERN (LEN / DIV / SWG / MLEN). Удержание + чёрная 1–6: mute.</td></tr>
<tr><td><kbd>GLO</kbd></td><td>Тап: глобальные страницы (MIDI OUT, дилей, реверб, хорус). Удержание + клавиши: смена паттерна или вариации, цепочка.</td></tr>
<tr><td><kbd>SAVE</kbd></td><td>Тап: карусель PROJECT / TOOLS / COPY. Удержание: карта переключения паттернов и вариаций; копирование трека.</td></tr>
<tr><td><kbd>ENV</kbd></td><td>Режим STEPS: белые клавиши становятся шагами на любой странице, чтобы ставить локи.</td></tr>
<tr><td><kbd>SEL</kbd></td><td>Тап: режим KEYS (клавиатура играет трек по нотам). Удержание + чёрная 1–6: выбор трека.</td></tr>
<tr><td><kbd>ARP</kbd></td><td>Только в комбинациях: + <kbd>EDIT</kbd> — страницы ARP; + чёрная 1–6 — solo.</td></tr>
<tr><td><kbd>PLAY</kbd> / <kbd>REC</kbd></td><td>Старт/стоп. REC: запись (см. раздел 6); удержание 0,4 с — FILL.</td></tr>
<tr><td><kbd>OCT-</kbd> / <kbd>OCT+</kbd></td><td>Зависят от экрана: банк шагов, октава, «назад/да», отмена правки (см. раздел 3).</td></tr>
<tr><td>Белые 1–16</td><td>Шаги текущего банка из 16 шагов. На HOME и страницах звука белые 1–6 (F3–D4) выбирают и проигрывают треки 1–6.</td></tr>
<tr><td>Чёрные 1–11</td><td>В комбинациях: 1–6 — треки, 1–8 — вариации. На PERFORM: эффекты.</td></tr>
</tbody></table>

<h3 id="splash">Заставка и стартовый экран</h3>
{{screen:fixes/splash_3_name.png|Заставка}}
<p>После включения около 3 с показывается заставка <code>FREITAKT</code> с подписью
<code>FORM FOLGT FUNCTION</code>. Любая клавиша, кнопка или ручка пропускает её и больше ничего не делает.</p>
<p>Затем загружается последний сохранённый паттерн, транспорт остановлен, на экране TRACKS.
Если паттерн загрузить не удалось, появляется вопрос <code>LOAD FAILED: SLOT A</code> / <code>START NEW PROJECT?</code>:
<kbd>OCT+</kbd> — да, <kbd>OCT-</kbd> — нет.</p>
<p>Если при включении держать <kbd>SEQ</kbd>, прошивка стартует в безопасном режиме (см. <a href="#safety">раздел 8</a>).</p>
{{/screen}}

<h2 id="quick">2. Быстрый старт</h2>
<ol>
<li><b>Включите</b> FM-1. После заставки откроется экран TRACKS со списком из шести треков.</li>
<li><b>Выберите трек</b>: повернуть <kbd class="k">ALGORITHM</kbd>, или <kbd>SEL</kbd> + чёрная 1–6,
или на TRACKS повернуть <kbd class="k">KNOB 1</kbd>. Выбранный трек помечен <code>&gt;</code>.</li>
<li><b>Поставьте шаги.</b> Нажмите <kbd>SEQ</kbd>, откроется страница PATTERN. Белые клавиши — 16 шагов выбранного трека:
тап включает или выключает шаг. Длинным треком (LEN &gt; 16) управляют банки: <kbd>OCT+</kbd> / <kbd>OCT-</kbd>.
То же можно делать прямо на TRACKS, пока ни один трек не взведён на запись.</li>
<li><b>Запустите</b>: <kbd>PLAY</kbd>. Повторное нажатие останавливает.</li>
<li><b>Смените машину.</b> <kbd>EDIT</kbd> открывает первую страницу звука, <kbd class="k">KNOB 1</kbd> (<code>MODEL</code>) выбирает машину,
<kbd class="k">KNOB 2–4</kbd> — её главные параметры. Быстрее: крутите <kbd class="k">PRESETS</kbd>, каждый щелчок — новый пресет.</li>
<li><b>Настройте шаг.</b> Вернитесь на <kbd>SEQ</kbd>, держите белую клавишу шага и крутите ручки:
<kbd class="k">KNOB 1</kbd> PROB (вероятность или условие), <kbd class="k">KNOB 2</kbd> RATCH (рэтчет 1–4),
<kbd class="k">KNOB 3</kbd> ACC (акцент), <kbd class="k">KNOB 4</kbd> NUDGE (сдвиг во времени).
Акцент также ставится через <kbd>OCT+</kbd> + шаг.</li>
<li><b>Темп и свинг.</b> <kbd>HOME</kbd> переключает на экран HOME: <kbd class="k">KNOB 1</kbd> — BPM, <kbd class="k">KNOB 2</kbd> — общий свинг.</li>
<li><b>Сохраните.</b> Остановите воспроизведение, тапните <kbd>SAVE</kbd> (страница <code>PROJECT 1/3</code>),
выберите букву паттерна <kbd class="k">KNOB 1</kbd> и поверните <kbd class="k">KNOB 4</kbd> два щелчка подряд
(первый показывает <code>AGAIN: SAVE</code>).</li>
</ol>
<div class="warn"><b>Автосохранения нет.</b> Правки живут в памяти до <kbd>SAVE</kbd>. После выключения
загрузится последняя сохранённая версия. Во время воспроизведения сохранить нельзя: <code>STOP TO SAVE</code>.</div>

<h2 id="screens">3. Экраны</h2>

<h3 id="rules">Общие правила</h3>
<ul>
<li><b>Страницы-списки.</b> Почти все экраны состоят из четырёх строк <code>ИМЯ значение</code>, по одной на ручку
<kbd class="k">KNOB 1–4</kbd> сверху вниз. Заголовка и номера страницы на экране нет: страницу узнают по строкам.
Номера в этом руководстве («FILTER 1») нужны только для ориентира.</li>
<li><b>Навигация.</b> Повторное нажатие кнопки раздела открывает следующую страницу по кругу.
<kbd class="k">SELECT</kbd> листает вперёд/назад без зацикливания. При возврате в раздел открывается страница, которая была в нём последней.
<kbd>EDIT</kbd> и <kbd>LFO</kbd> срабатывают на нажатии, <kbd>FX</kbd>, <kbd>GLO</kbd>, <kbd>SAVE</kbd> и <kbd>SEQ</kbd> — на отпускании
(так их можно держать для комбинаций).</li>
<li><b>Всплывающее окно ручки.</b> При повороте ручки весь экран показывает имя параметра и крупное значение
(значение лока — инверсно). Окно закрывается через 3 с после последнего поворота.
Пока оно открыто, <kbd>OCT-</kbd> — <b>отмена</b>: параметр возвращается к значению до начала поворотов.</li>
<li><b>Белые клавиши</b> на HOME и на страницах трека (EDIT, FX кроме COMP, LFO) — белые 1–6 выбирают и проигрывают треки 1–6.
На PATTERN и TRACKS белые клавиши — шаги. На COMP — переключатели DUCK.</li>
<li><b>Верхняя строка</b> (на страницах-списках): справа батарея; левее замок, если включён режим STEPS (<kbd>ENV</kbd>).
Слева значок записи: полный — выбранный трек взведён, кольцо — взведён другой трек.
Дальше текст: строка MIDI LEARN, карта переключения <code>CUE A1*</code>, <code>FILL</code> или <code>LOCK STEP n</code>.</li>
</ul>

{{shot:popup.png|Всплывающее окно ручки}}

<h3 id="home">HOME</h3>
{{screen:00_home.png|HOME}}
<p><b>Как открыть:</b> тап <kbd>HOME</kbd>. Если открыт TRACKS — ещё один тап <kbd>HOME</kbd>.</p>
<p>Минимальный экран для игры: вверху загрузка процессора <code>CPU</code> и батарея, крупно темп <code>BPM 120</code>
и строка состояния <code>P1 V2 T6 L1</code>:</p>
<ul>
<li><code>P</code> — паттерн (1–4 = A–D; <code>P-</code> — ничего не загружено);</li>
<li><code>V</code> — вариация;</li>
<li><code>T</code> — выбранный трек;</li>
<li><code>L</code> — текущий банк шагов.</li>
</ul>
<p>Пока играет цепочка, ниже выводится строка <code>CH A1 B3 …</code> (до 6 звеньев): текущее звено инверсно, следующее подчёркнуто.</p>
<p>При внешнем клоке вместо <code>BPM</code> показывается источник и измеренный темп: <code>USB 120</code>, <code>TRS 98</code>.</p>
<table><thead><tr><th>Орган</th><th>Действие</th></tr></thead><tbody>
<tr><td><kbd class="k">KNOB 1</kbd></td><td>BPM 40–240. При внешнем клоке показывает <code>CLK USB</code> и т. п., темп не меняется.</td></tr>
<tr><td><kbd class="k">KNOB 2</kbd></td><td>Общий SWING 0–100 % (видно только во всплывающем окне).</td></tr>
<tr><td><kbd class="k">KNOB 3/4</kbd>, <kbd class="k">SELECT</kbd></td><td>Ничего.</td></tr>
<tr><td>Белые 1–6</td><td>Выбрать и проиграть трек 1–6.</td></tr>
</tbody></table>
{{/screen}}
<div class="shots">{{shot:popup_home_bpm.png|HOME: поворот KNOB 1}}{{shot:home_clk_usb.png|HOME при клоке по USB}}</div>
<p>В режиме MIDI-клавиатуры строка состояния HOME меняется на <code>MIDI KBD CH1 OCT+1</code> (см. <a href="#midikbd">раздел 6</a>).</p>

<h3 id="tracks">TRACKS (микшер)</h3>
{{screen:24_tracks.png|TRACKS}}
<p><b>Как открыть:</b> тап <kbd>HOME</kbd> на экране HOME. Этот экран открывается после включения.</p>
<p>Шесть строк, по одной на трек: номер, отметка записи (квадрат: залитый, пока играет; контур, пока стоит),
машина и справа мелко имя пресета (<code>*</code> — пресет изменён). Выбранный трек отмечен <code>&gt;</code>.
Заглушённый трек (MUTE, LEVEL 0 или solo другого трека) показан инверсно.</p>
<table><thead><tr><th>Орган</th><th>Действие</th></tr></thead><tbody>
<tr><td><kbd class="k">KNOB 1</kbd></td><td>Выбор трека.</td></tr>
<tr><td><kbd class="k">KNOB 2</kbd></td><td>LEVEL трека. Первый поворот на заглушённом треке только снимает mute.</td></tr>
<tr><td><kbd class="k">KNOB 3</kbd></td><td>LEN трека (шаги).</td></tr>
<tr><td><kbd class="k">KNOB 4</kbd></td><td>PAN.</td></tr>
<tr><td><kbd class="k">SELECT</kbd></td><td>Темп (BPM) с ускорением.</td></tr>
<tr><td>Белые 1–16</td><td>Шаги выбранного трека, пока ни один трек не взведён на запись.</td></tr>
<tr><td><kbd>OCT-</kbd> / <kbd>OCT+</kbd></td><td>Предыдущий / следующий банк шагов (если ни один трек не взведён).</td></tr>
<tr><td><kbd>REC</kbd> (тап)</td><td>Взвести / снять запись выбранного трека.</td></tr>
</tbody></table>
{{/screen}}
{{shot:tracks_muted.png|TRACKS: T3 заглушён, выбран T2}}

<h3 id="sound">EDIT: страницы SOUND</h3>
<div class="shots">{{shot:01_sound.png|SOUND 1 (K909)}}{{shot:02_sound_2.png|SOUND 2}}{{shot:03_sound_3.png|SOUND 3}}{{shot:93_sound_kboom.png|SOUND 1 машины BOOM}}</div>
<p><b>Как открыть:</b> <kbd>EDIT</kbd>; повторные нажатия листают страницы SOUND, а за ними ARP. Удержание <kbd>EDIT</kbd> 0,4 с открывает список пресетов.</p>
<p>Вверху нарисована форма удара трека. Страницы — список параметров по четыре на страницу:
<code>MODEL</code>, четыре главных ручки машины, её дополнительные ручки, затем
<code>LVL</code> (громкость), <code>PAN</code>, <code>NOTE</code> (MIDI-нота трека), <code>CHOKE</code> (группа глушения OFF/1–4)
и <code>PRIO</code> (приоритет голоса HIGH/LOW). Число страниц зависит от машины:
3 у простых машин, 4 у машин с 7–8 ручками (ACID, FM, SKIN, *EFM, большинство *TRX), 5 у SAMPL.</p>
<p><code>MODEL</code> показывает машину и имя пресета. Поворот <kbd class="k">KNOB 1</kbd> перебирает 45 машин по кругу:</p>
<p class="mono">BDTRX SDTRX TMTRX CGTRX CPTRX RSTRX CBTRX CHTRX OHTRX CYTRX SHTRX CLTRX B2TRX ·
BDEFM SDEFM TMEFM CPEFM RSEFM CBEFM HHEFM CYEFM · BOOM PUNCH SNAP CRACK METAL HISS SKIN FM ACID SAMPL ·
K808 K909 S808 S909 C808 C909 HATC HATO CYMB COWB TOM CONGA RIM CLAVE</p>
<p>У всех машин первые ручки — <code>TUNE</code> и <code>DECAY</code>. Белые 1–6 выбирают трек.
С удержанным шагом в режиме STEPS ручки ставят parameter lock, а <code>MODEL</code> — machine lock (см. <a href="#locks">раздел 4</a>).</p>

<h3 id="presets">Пресеты</h3>
<div class="shots">{{shot:presets/06_list.png|Список пресетов (удержание EDIT)}}{{shot:presets/01_popup.png|Экран пресетов (энкодер PRESETS)}}</div>
<p><b>Энкодер <kbd class="k">PRESETS</kbd></b> на любой странице загружает в выбранный трек следующий или предыдущий пресет
и показывает отдельный экран на весь дисплей (страница под ним скрыта): сверху банк и трек, в середине
крупно на инверсной полосе текущий пресет, по три пресета выше и ниже, справа названия машин; пунктир — граница банка.
Экран исчезает через 3 с после последнего щелчка (или сразу при другом действии — смене трека, открытии списка,
меню), и страница рисуется заново целиком.</p>
<p><b>Список пресетов</b> (удержание <kbd>EDIT</kbd> 0,4 с) — банки MD / MODERN / CLASSIC / USER (96 пользовательских ячеек):</p>
<table><thead><tr><th>Орган</th><th>Действие</th></tr></thead><tbody>
<tr><td><kbd class="k">KNOB 1</kbd> / <kbd class="k">PRESETS</kbd></td><td>Курсор; выбранный пресет сразу звучит (прослушивание).</td></tr>
<tr><td><kbd class="k">KNOB 2</kbd> / <kbd class="k">SELECT</kbd></td><td>Банк.</td></tr>
<tr><td><kbd class="k">ALGORITHM</kbd>, белые 1–6</td><td>Трек.</td></tr>
<tr><td><kbd>OCT+</kbd></td><td>LOAD — загрузить в трек.</td></tr>
<tr><td><kbd>SAVE</kbd></td><td>Сохранить звук трека в новую ячейку USER; повторное нажатие в течение 1,5 с — поверх выбранной.</td></tr>
<tr><td><kbd>OCT-</kbd> дважды</td><td>Удалить пользовательский пресет.</td></tr>
<tr><td><kbd>EDIT</kbd> / <kbd>HOME</kbd></td><td>Выйти. <kbd>PLAY</kbd> работает как обычно.</td></tr>
</tbody></table>

<h3 id="arp">EDIT: страницы ARP (арпеджиатор и ретриггер)</h3>
{{screen:04_arp.png|ARP 1}}
<p><b>Как открыть:</b> держите <kbd>ARP</kbd> и нажмите <kbd>EDIT</kbd>. Следующее <kbd>ARP</kbd> + <kbd>EDIT</kbd> откроет ARP 2.
Страницы ARP также идут в цикле <kbd>EDIT</kbd> сразу после SOUND.</p>
<table><thead><tr><th>Ручка</th><th>ARP 1</th><th>ARP 2 (только у ACID, SAMPL, FM, SKIN)</th></tr></thead><tbody>
<tr><td>1</td><td><code>MODE</code>: OFF / RTRG / ARP1 / ARP2 (ARP1 и ARP2 — только у машин с высотой тона)</td><td><code>DIR</code>: UP DOWN UPDN RND</td></tr>
<tr><td>2</td><td><code>RATE</code>: 1/4 4T 1/8 8T 1/16 16T 1/32 (по умолчанию 1/16)</td><td><code>CHORD</code>: OCT 5TH MAJ MIN SUS4 MAJ7 MIN7 DOM7 (ARP2 его не использует)</td></tr>
<tr><td>3</td><td><code>GATE</code>: 0–100 % (50)</td><td><code>RANGE</code>: 1–4 октавы</td></tr>
<tr><td>4</td><td><code>FADE</code>: −64…+63 (затухание или нарастание повторов)</td><td>—</td></tr>
</tbody></table>
{{/screen}}

<h3 id="fx">FX: фильтр, посылы, инсерт, компрессор</h3>
<p><b>Как открыть:</b> тап <kbd>FX</kbd> (срабатывает на отпускании); повторные тапы листают 7 страниц по кругу.
Удержание <kbd>FX</kbd> 0,4 с открывает PERFORM (см. ниже). Страницы 1–5 относятся к выбранному треку, COMP — общий.</p>
<div class="shots">{{shot:05_filter.png|FILTER 1}}{{shot:06_filter_2.png|FILTER 2}}{{shot:07_fx.png|FX (посылы)}}{{shot:08_insert.png|INSERT 1}}{{shot:09_insert_2.png|INSERT 2}}{{shot:10_comp.png|COMP 1}}{{shot:11_comp_2.png|COMP 2}}</div>
<table><thead><tr><th>Страница</th><th>KNOB 1</th><th>KNOB 2</th><th>KNOB 3</th><th>KNOB 4</th></tr></thead><tbody>
<tr><td>FILTER 1</td><td><code>TYPE</code> OFF LP BP HP NOT</td><td><code>CUT</code> срез (до 16 кГц)</td><td><code>RESO</code> 0–127</td><td><code>ENV</code> −64…+63 (огибающая на срез)</td></tr>
<tr><td>FILTER 2</td><td><code>DECAY</code> огибающей фильтра</td><td>—</td><td>—</td><td>—</td></tr>
<tr><td>FX (посылы)</td><td><code>DST</code> дисторшн</td><td><code>CHO</code> хорус</td><td><code>DLY</code> дилей</td><td><code>REV</code> реверб</td></tr>
<tr><td>INSERT 1</td><td><code>TYPE</code> OFF SOFT HARD FOLD FUZZ CRUSH PHASR FLANG CHOR</td><td colspan="3">A / B / C, названия зависят от TYPE:
SOFT/HARD/FOLD/FUZZ — DRIVE TONE LEVEL; CRUSH — BITS RATE LPF; PHASR/FLANG/CHOR — RATE DEPTH FDBK</td></tr>
<tr><td>INSERT 2</td><td><code>MIX</code> (100 %)</td><td><code>SRR</code> понижение частоты дискретизации (OFF или частота)</td><td>—</td><td>—</td></tr>
<tr><td>COMP 1</td><td><code>SRC</code> OFF / T1–T6 — трек-источник дакинга</td><td><code>THRSH</code></td><td><code>RATIO</code></td><td><code>REL</code></td></tr>
<tr><td>COMP 2</td><td><code>ATK</code></td><td><code>KNEE</code> HARD/SOFT</td><td><code>MKUP</code> подъём</td><td><code>GHOST</code> MUTE / KEEP / HIDE</td></tr>
</tbody></table>
<p>На страницах COMP белые 1–6 включают и выключают <b>DUCK</b> для треков 1–6, то есть какие треки «приседают» от источника.
<code>GHOST</code> задаёт поведение заглушённого источника: MUTE — дакинга нет, KEEP — дакинг остаётся,
HIDE — источник никогда не слышен, только качает остальные.</p>

<h3 id="perform">PERFORM</h3>
{{screen:perform/map.png|PERFORM}}
<p><b>Как открыть:</b> держите <kbd>FX</kbd> 0,4 с (или <kbd>FX</kbd> + клавиша/ручка); экран живёт, пока <kbd>FX</kbd> удержана.
В меню пункт <code>PERFORM</code> = <code>PAGE</code> оставляет экран открытым после отпускания;
закрывается тапом <kbd>HOME</kbd> или сменой страницы.</p>
<table><thead><tr><th>Орган</th><th>Действие</th></tr></thead><tbody>
<tr><td><kbd class="k">KNOB 1–4</kbd></td><td>FILT (0 — выкл., влево LPF, вправо HPF), CRUSH, THROW (вброс в дилей/реверб), DEPTH (при OCT UP/DN — SHIMR)</td></tr>
<tr><td>Чёрные 1–10</td><td>REPEAT 1/8, 1/16, 1/32, REVERSE, TAPE STOP, LPF, HPF, FREEZE, OCT UP, OCT DN — пока клавиша удержана</td></tr>
<tr><td>Белые 1–6</td><td>REPEAT STEP: повтор текущего шага трека 1–6</td></tr>
<tr><td>Белые 13–16</td><td>Частота повтора 1/4, 1/8, 1/16, 1/32</td></tr>
<tr><td><kbd class="k">SELECT</kbd>, <kbd class="k">ALGORITHM</kbd>, <kbd class="k">PRESETS</kbd></td><td>Игнорируются</td></tr>
</tbody></table>
{{/screen}}

<h3 id="pattern">SEQ: PATTERN</h3>
{{screen:12_pattern.png|PATTERN}}
<p><b>Как открыть:</b> <kbd>SEQ</kbd> (на отпускании). В разделе SEQ одна страница.</p>
<table><thead><tr><th>Ручка</th><th>Строка</th><th>Значения</th></tr></thead><tbody>
<tr><td>1</td><td><code>LEN</code></td><td>Длина трека 1–64 шага (16)</td></tr>
<tr><td>2</td><td><code>DIV</code></td><td>4BAR 2BAR 1/1 1/2 1/4 1/8 8T <b>1/16</b> 16T 1/32, затем FIT: F4BR F2BR FBAR F1/2</td></tr>
<tr><td>3</td><td><code>SWG</code></td><td>Свинг трека 0–100 %</td></tr>
<tr><td>4</td><td><code>MLEN</code></td><td>Master Length: OFF или 2–1024 шестнадцатых (общий для паттерна)</td></tr>
</tbody></table>
<p>Белые клавиши — шаги выбранного трека. Удержанный шаг и ручки задают PROB / RATCH / ACC / NUDGE
(см. <a href="#steps">раздел 4</a>). <kbd>OCT-</kbd>/<kbd>OCT+</kbd> переключают банк.</p>
{{/screen}}
{{shot:seq/20_step_held_prob.png|Удержанный шаг + KNOB 1 (PROB)}}

<h3 id="lfo">LFO</h3>
<div class="shots">{{shot:21_lfo.png|LFO 1}}{{shot:22_lfo_2.png|LFO 2 (страница)}}{{shot:23_dest.png|DEST}}</div>
<p><b>Как открыть:</b> <kbd>LFO</kbd>; повторные нажатия листают 3 страницы. У каждого трека один LFO с четырьмя назначениями.</p>
<table><thead><tr><th>Страница</th><th>KNOB 1</th><th>KNOB 2</th><th>KNOB 3</th><th>KNOB 4</th></tr></thead><tbody>
<tr><td>1 (с формой волны)</td><td><code>WAVE</code> SQUAR SAW RSAW SINE TRI S&amp;H WANDR EXP+ EXP- RWALK</td><td><code>RATE</code> (в SYNC: от 8BAR до 1/64, с пунктирными и триолями)</td><td><code>MORPH</code></td><td><code>PHASE</code></td></tr>
<tr><td>2</td><td><code>TRIG</code> FREE / HIT / PLAY</td><td><code>MODE</code> SYNC / HZ / TIME</td><td>—</td><td>—</td></tr>
<tr><td>DEST</td><td><code>SLOT</code> назначение 1–4</td><td><code>SRC</code> LFO1 / OFF</td><td><code>DEST</code> куда</td><td><code>DEPTH</code> −64…+64</td></tr>
</tbody></table>
<p>DEST может быть: ручка машины, LVL, PAN, F.CUT, F.RES, SRR, посылы DST/CHO/DLY/REV, MIX и A/B/C инсерта.
На страницах LFO <kbd>OCT+</kbd> переключает режим RATE: SYNC → HZ → TIME. TRIG: FREE — свободный бег, HIT — рестарт на каждом ударе трека,
PLAY — рестарт по PLAY.</p>

<h3 id="glo">GLO: MIDI OUT, дилей, реверб, хорус</h3>
<p><b>Как открыть:</b> короткий тап <kbd>GLO</kbd> (меньше 0,4 с, без клавиш); повторные тапы листают 5 страниц.
Удержание <kbd>GLO</kbd> с клавишами управляет паттернами (см. <a href="#patterns">раздел 5</a>).</p>
<div class="shots">{{shot:13_midi out.png|MIDI OUT}}{{shot:14_dly.png|DLY 1}}{{shot:15_dly_2.png|DLY 2}}{{shot:16_reverb.png|REVERB}}{{shot:17_chorus.png|CHORUS}}</div>
<table><thead><tr><th>Страница</th><th>KNOB 1</th><th>KNOB 2</th><th>KNOB 3</th><th>KNOB 4</th></tr></thead><tbody>
<tr><td>MIDI OUT</td><td><code>OUT</code> OFF / USB (/ BLE / BOTH)</td><td><code>T1CH</code>…<code>T6CH</code> канал выбранного трека, OFF/1–16</td><td><code>KBD</code> OFF / TRK / 1–16</td><td><code>LOCL</code> ON/OFF</td></tr>
<tr><td>DLY 1</td><td><code>TIME</code> (1/8; есть пунктирные и триоли)</td><td><code>FDBK</code></td><td><code>COLR</code></td><td><code>MIX</code></td></tr>
<tr><td>DLY 2</td><td><code>MODE</code> NORM / PING</td><td>—</td><td>—</td><td>—</td></tr>
<tr><td>REVERB</td><td><code>TYPE</code> ROOM / SPRNG / HALL</td><td><code>SIZE</code></td><td><code>DAMP</code></td><td>—</td></tr>
<tr><td>CHORUS</td><td><code>CRT</code> частота</td><td><code>CDP</code> глубина</td><td>—</td><td>—</td></tr>
</tbody></table>
<div class="shots">{{shot:glo_delay_time_dotted.png|DLY: пунктирное время}}{{shot:glo_reverb_hall.png|REVERB: HALL}}</div>

<h3 id="save">SAVE: PROJECT, TOOLS, COPY</h3>
<p><b>Как открыть:</b> тап <kbd>SAVE</kbd>; повторные тапы листают карусель из трёх страниц (<code>PROJECT 1/3</code>,
<code>TOOLS 2/3</code>, <code>COPY 3/3</code>). В карусели крупно показан текущий пункт между стрелками, под ним значение и пояснение,
внизу подсказки по кнопкам. <kbd class="k">SELECT</kbd> прокручивает пункты по кругу.</p>
<p><b>Защита от случайностей:</b> LOAD, SAVE, PASTE и все действия TOOLS требуют двух щелчков ручки в течение примерно 1,5 с.
Первый щелчок показывает <code>AGAIN: …</code>.</p>
<div class="shots">{{shot:18_project.png|PROJECT}}{{shot:19_tools.png|TOOLS}}{{shot:20_copy.png|COPY}}</div>
<table><thead><tr><th>Страница</th><th>KNOB 1</th><th>KNOB 2</th><th>KNOB 3</th><th>KNOB 4</th></tr></thead><tbody>
<tr><td>PROJECT</td><td>Паттерн A–D (на экране <code>SLOT</code>); значение — имя, <code>EMPTY</code> или <code>USED</code></td><td><code>NAME</code>: поворот вправо — переименовать</td><td><code>LOAD</code> ×2</td><td><code>SAVE</code> ×2</td></tr>
<tr><td>TOOLS</td><td>CLEAR TRACK: шаги, motion и локи трека</td><td>INIT SOUND: звук трека по умолчанию</td><td>CLEAR ALL: все треки (шаги, LEN, DIV, SWG)</td><td>INIT ALL: всё (сохранённые паттерны и настройки остаются)</td></tr>
<tr><td>COPY</td><td>Паттерн A–D</td><td><code>COPY</code></td><td><code>PASTE</code> ×2</td><td><code>LEARN</code> — MIDI LEARN вкл/выкл</td></tr>
</tbody></table>
<p>На странице TOOLS каждое действие делает своя ручка: первый поворот показывает действие и <code>AGAIN:</code>, второй выполняет.</p>
<p><b>Вопросы.</b> SAVE в другую букву, если в памяти несохранённые правки, спрашивает <code>OVERWRITE B*?</code>;
LOAD с несохранёнными правками спрашивает <code>DISCARD EDITS?</code>. <kbd>OCT+</kbd> — YES, <kbd>OCT-</kbd> — NO.</p>
<div class="shots">{{shot:save_project_overwrite.png|OVERWRITE?}}{{shot:save_project_discard.png|DISCARD EDITS?}}</div>
<p><b>Имя</b> (<kbd class="k">KNOB 2</kbd> на PROJECT): белые клавиши вводят буквы мультитапом, чёрные — F# ←, G# пробел, A# →, C# удалить, D# ABC/123;
<kbd class="k">KNOB 1</kbd> двигает курсор, <kbd class="k">KNOB 2</kbd> меняет символ; <kbd>OCT+</kbd> — записать, <kbd>OCT-</kbd> или <kbd>HOME</kbd> — отмена.</p>

<h3 id="menu">Меню устройства</h3>
<div class="shots">{{shot:90_menu.png|MENU: COLOR}}{{shot:91_menu_clock.png|MENU: CLOCK}}</div>
<p><b>Как открыть:</b> держите <kbd>HOME</kbd> 0,7 с. Закрыть — <kbd>OCT-</kbd> или снова удержание <kbd>HOME</kbd>.
<kbd class="k">SELECT</kbd> прокручивает пункты по кругу, <kbd class="k">KNOB 1</kbd> задаёт значение, <kbd>OCT+</kbd> переключает его по кругу.
Настройки устройства сохраняются отдельно от паттернов.</p>
<table><thead><tr><th>Пункт</th><th>Значения</th><th>Смысл</th></tr></thead><tbody>
<tr><td>COLOR</td><td>8 тем (ORANGE по умолчанию)</td><td>Цвет экрана</td></tr>
<tr><td>SPEAKER EQ</td><td>FLAT / LOWCUT / BASS+</td><td>Коррекция для встроенного динамика</td></tr>
<tr><td>USB LEVEL</td><td>MASTER / FIXED</td><td>Следует ли USB-аудио ручке MASTER</td></tr>
<tr><td>PERFORM</td><td>HOLD / PAGE</td><td>PERFORM только пока FX удержана, или остаётся открытым</td></tr>
<tr><td>ACCEL</td><td>OFF / ON</td><td>Ускорение ручек</td></tr>
<tr><td>BACKLIGHT</td><td>OFF DIM LO DIM HI INV INV LO INV HI</td><td>Подсветка клавиш</td></tr>
<tr><td>CLOCK</td><td>INT / USB / TRS (/ BLE / LINK)</td><td>Источник темпа</td></tr>
<tr><td>MIDI SYNC</td><td>OFF / USB (/ BLE / BOTH)</td><td>Отправка MIDI-клока и старт/стоп</td></tr>
<tr><td>MIDI CHANNEL</td><td>1–16 (10)</td><td>Входной канал</td></tr>
<tr><td>MIDI ROUTE</td><td>NOTE / SEL / TRACK</td><td>Куда идут входящие ноты: NOTE — ноты канала MIDI CHANNEL играют трек с таким же NOTE; SEL — выбранный трек хроматически; TRACK — каналы 1–6 играют треки 1–6 хроматически</td></tr>
<tr><td>USB MIDI</td><td>только чтение</td><td>Состояние USB-соединения</td></tr>
<tr><td>METRONOME</td><td>OFF / LOW / MID / HIGH</td><td>Щелчок на четвертях, пока идёт транспорт</td></tr>
<tr><td>MUTE MODE</td><td>NOW / BAR</td><td>Mute сразу или со следующего такта</td></tr>
<tr><td>MIDI THRU</td><td>OFF / ON</td><td>Пропуск входящего MIDI на выход</td></tr>
<tr><td>REC QUANT</td><td>1/16 / 1/8 / 1/32 / OFF</td><td>Квантование live-записи</td></tr>
<tr><td>HARDWARE CALIBRATION</td><td><kbd>OCT+</kbd> START</td><td>Обучение кнопок и ручек</td></tr>
<tr><td>BLUETOOTH</td><td><kbd>OCT+</kbd> OPEN</td><td>Страница Bluetooth (только в сборке с Bluetooth)</td></tr>
</tbody></table>

<h2 id="seq">4. Секвенсор</h2>

<h3 id="steps">Шаги, банки, свойства шага</h3>
<p>У каждого трека до <b>64 шагов</b>. Белые клавиши показывают 16 шагов текущего банка, число банков зависит от LEN
(LEN 20 — два банка). Белые клавиши работают как шаги на PATTERN, на TRACKS (пока ни один трек не взведён) и
на любой странице в режиме STEPS (<kbd>ENV</kbd>).</p>
<ul>
<li><b>Тап</b> по шагу включает или выключает его (на отпускании). Выключенный шаг сбрасывается в обычный.</li>
<li><b>Удержание</b> 0,3 с включает выключенный шаг и делает его «удержанным»: ручки меняют свойства шага.</li>
<li><kbd>OCT+</kbd> + шаг ставит или снимает <b>акцент</b> (выключенный шаг включается с акцентом). Пока <kbd>OCT+</kbd> удержана,
горят только акцентированные шаги.</li>
<li><b>Банки:</b> на PATTERN и TRACKS <kbd>OCT-</kbd> — предыдущий банк (сразу), <kbd>OCT+</kbd> — следующий (на отпускании,
если не ставили акцент). На других страницах в режиме STEPS банк меняет <kbd>ENV</kbd> + <kbd>OCT-</kbd>/<kbd>OCT+</kbd>.</li>
</ul>
<table><thead><tr><th>Удержанный шаг + ручка (PATTERN, TRACKS)</th><th>Значения</th></tr></thead><tbody>
<tr><td><kbd class="k">KNOB 1</kbd> PROB</td><td>0 %, 5–100 % шагом 5 %, 1-SHOT (один раз после PLAY), условия A/B: 1/2 … 8/8 (сыграть в A-м из каждых B проходов), FILL, !FILL</td></tr>
<tr><td><kbd class="k">KNOB 2</kbd> RATCH</td><td>1–4 удара на шаг</td></tr>
<tr><td><kbd class="k">KNOB 3</kbd> ACC</td><td>Вправо — акцент, влево — без акцента (громкость 127 вместо 96)</td></tr>
<tr><td><kbd class="k">KNOB 4</kbd> NUDGE</td><td>−50…+50 сотых шага: раньше / позже</td></tr>
</tbody></table>
<p><b>FILL:</b> держите <kbd>REC</kbd> 0,4 с на любой странице. Пока кнопка удержана, играют шаги с условием FILL, а шаги !FILL молчат.
В верхней строке горит <code>FILL</code>.</p>

<h3 id="len">LEN, DIV, FIT: полиметрия и полиритмия</h3>
<p>У каждого трека своя длина <code>LEN</code> и свой шаг сетки <code>DIV</code> (страница PATTERN). Длина шага в долях:
1/16 = ¼ доли, 1/8 = ½, 1/4 = 1, 8T = ⅓, 16T = ⅙, 1/32 = ⅛, 1/2 = 2, 1/1 = 4, 2BAR = 8, 4BAR = 16.</p>
<p><b>FIT-деления</b> (F1/2, FBAR, F2BR, F4BR) растягивают <i>все</i> LEN шагов трека ровно на 2, 4, 8 или 16 долей,
без накопления ошибки. Если поменять LEN во время игры, трек пересчитывается под новую длину.</p>
<div class="note"><b>Пример полиметрии.</b> T1: LEN 16, 1/16. T2: LEN 12, 1/16. MLEN OFF. T2 повторяется каждые 3 доли,
T1 — каждый такт; совпадают через 3 такта.<br>
<b>Пример полиритмии.</b> T1: LEN 4, 1/16 (четыре удара на долю). T2: LEN 3, FBAR — три равных удара на такт (триоль половинными).
LEN 5 F1/2 — квинтоль на полтакта; LEN 7 F4BR — 7 шагов на 4 такта.</div>
<p><b>Master Length</b> (<code>MLEN</code>, общий для паттерна) считает шестнадцатые от старта. Каждые MLEN шестнадцатых
все треки начинаются заново с первого шага, даже если ничего не стоит в очереди. Пример: при MLEN 16 трек
LEN 12 играет шаги 1–12, затем 1–4, и всё рестартует.
Без MLEN треки бегут свободно (настоящая полиметрия). Отложенная смена паттерна при MLEN происходит на его границе,
а без MLEN — в конце самого длинного трека (LEN × DIV).</p>

<h3 id="swing">Свинг</h3>
<p>Свингов два: общий (HOME, <kbd class="k">KNOB 2</kbd>) и свой у каждого трека (PATTERN, <code>SWG</code>).
Они складываются, максимум 100. Чётные шаги сдвигаются позже, нечётные раньше: SWG 50 даёт пары 1,2 / 0,8 шага,
SWG 100 — 1,4 / 0,6. С FIT-делением мера — длина своего шага.</p>

<h3 id="locks">Parameter lock и machine lock</h3>
{{screen:locks/sound_held.png|Шаг 5 удержан на странице SOUND}}
<ol>
<li>Нажмите <kbd>ENV</kbd> (на отпускании), чтобы включить режим <b>STEPS</b>. Загорится ENV, в верхней строке появится замок.
Белые клавиши — шаги выбранного трека на любой странице.</li>
<li>Откройте страницу с нужным параметром (EDIT, FX, LFO, ARP; на TRACKS — LEVEL и PAN).</li>
<li>Держите шаг и крутите ручку. Шаг включится, если был выключен, а значение запишется только в него.
В верхней строке видно <code>LOCK STEP n</code>, значение лока показывается инверсно.</li>
</ol>
<p><b>Machine lock:</b> держите шаг и крутите <code>MODEL</code> (SOUND, <kbd class="k">KNOB 1</kbd>). Сообщение — <code>MLOCK STEP n МАШИНА</code>.
ACID пропускается. Возврат к собственной машине трека снимает machine lock (<code>MLOCK STEP n OFF</code>).</p>
<p><b>Что можно залочить:</b> ручки машины, LVL, PAN, посылы, CUT/RESO/ENV/DECAY фильтра, SRR, A/B/C/MIX инсерта,
ATK/SUS/REL/LOOP/PLAY сэмплера, RATE/MORPH/DEPTH LFO, все параметры ARP, MODEL (machine lock).
Нельзя: типы (фильтра, инсерта), глобальные параметры. Ручка, которую залочить нельзя, при удержанном шаге ничего не делает.</p>
<p><b>Как стереть:</b></p>
<ul>
<li>шаг + тап <kbd>OCT-</kbd> — локи этого шага на четырёх ручках текущей страницы (<code>STEP n LOCKS CLEARED</code>);</li>
<li>шаг + удержание <kbd>OCT-</kbd> 0,6 с — все локи шага (<code>STEP n ALL CLEARED</code>);</li>
<li><kbd>REC</kbd> + <kbd>SEL</kbd> 5 с — весь трек (см. ниже).</li>
</ul>
<p>На паттерн отводится 1024 события локов и motion на все треки; при переполнении — <code>LOCKS FULL</code> / <code>MOTION FULL</code>.</p>
{{/screen}}
{{shot:popup_lock.png|Значение лока во всплывающем окне — инверсно}}

<h3 id="arpseq">ARP и RTRG в секвенсоре</h3>
<p><b>RTRG</b> (ретриггер) — дробь одного удара на всё время шага с частотой RATE. <b>ARP1</b> (только ACID, SAMPL, FM, SKIN) играет аккорд CHORD
от ноты шага в направлении DIR на RANGE октав. <b>ARP2</b> — классический арпеджиатор по удержанным клавишам в режиме KEYS; на шаге (одна нота) звучит как ARP1.
Старое значение ARP в проектах открывается как ARP1; на барабанах ARP1/ARP2 звучат как RTRG. Ролл длится от удара до конца шага (с учётом TIE) или до следующего удара и заменяет RATCH.
RATE считается от длины шага трека (со свингом и FIT). FADE гасит или поднимает повторы. Все параметры ARP можно лочить по шагам.</p>

<h3 id="leds">Светодиоды клавиш</h3>
<table><thead><tr><th>Состояние</th><th>Белые (шаги)</th><th>Чёрные 1–6</th></tr></thead><tbody>
<tr><td><b>Горит</b></td><td>Шаг включён</td><td>Выбранный трек</td></tr>
<tr><td><b>Мигает</b> (250 мс)</td><td>У шага есть parameter lock или machine lock</td><td>—</td></tr>
<tr><td><b>Дышит</b> (плавно)</td><td>—</td><td>Трек взведён на запись</td></tr>
<tr><td><b>Подмигивает</b> (горит с короткой паузой раз в секунду)</td><td>—</td><td>Выбранный и взведённый трек</td></tr>
<tr><td>Инверсия</td><td>Бегущий курсор (текущий шаг выбранного трека)</td><td>—</td></tr>
<tr><td>Темно</td><td>Шаг выключен или за пределами LEN</td><td>—</td></tr>
</tbody></table>
<p>Клавиши 1, 5, 9, 13 слегка подсвечены как метки долей (зависит от BACKLIGHT). Пока удержана кнопка комбинации, чёрные 1–6 показывают:
<kbd>SEQ</kbd> — слышимые треки (mute в ожидании такта мигает), <kbd>ARP</kbd> — solo (F#5 горит, если MOMENTARY), <kbd>SEL</kbd> — выбранный трек.
Кнопки: <kbd>PLAY</kbd> мигает при игре, <kbd>REC</kbd> горит, если что-то взведено, <kbd>SEL</kbd> — режим KEYS, <kbd>ENV</kbd> — STEPS,
<kbd>OCT-</kbd>/<kbd>OCT+</kbd> дышат, если есть предыдущий/следующий банк.</p>

<h3 id="copy">Копирование, вставка, очистка</h3>
<table><thead><tr><th>Комбинация</th><th>Что делает</th></tr></thead><tbody>
<tr><td>шаг(и) + <kbd>REC</kbd></td><td>Скопировать шаги от младшего до старшего удержанного (в пределах LEN) с локами: <code>COPIED 3-8</code></td></tr>
<tr><td>шаг + <kbd>PLAY</kbd></td><td>Вставить копию целиком с этого шага; всё, что за LEN, отбрасывается</td></tr>
<tr><td>несколько шагов + <kbd>PLAY</kbd></td><td>Заполнить диапазон: копия повторяется или обрезается. Старые локи целевых шагов стираются</td></tr>
<tr><td><kbd>SAVE</kbd> + <kbd>REC</kbd></td><td>Скопировать выбранный трек (шаги, локи, LEN/DIV/SWG): <code>COPIED T3</code></td></tr>
<tr><td><kbd>SAVE</kbd> + <kbd>PLAY</kbd></td><td>Вставить в выбранный трек: <code>PASTED T5</code></td></tr>
<tr><td><kbd>GLO</kbd> + <kbd>REC</kbd></td><td>Скопировать паттерн (все треки текущей вариации)</td></tr>
<tr><td><kbd>GLO</kbd> + <kbd>PLAY</kbd></td><td>Вставить паттерн в текущую вариацию</td></tr>
<tr><td><kbd>REC</kbd> + <kbd>SEL</kbd> 5 с</td><td>Очистить выбранный трек (шаги, motion, локи). Идёт отсчёт <code>CLEAR T1</code> 5→1; отпустите любую кнопку — отмена</td></tr>
<tr><td>SAVE → COPY</td><td>Копирование паттерна в другую букву (A–D)</td></tr>
</tbody></table>
<p>В этих комбинациях REC не взводит запись и не включает FILL, PLAY не запускает транспорт. Буфер обмена живёт до выключения.</p>
{{shot:92_clear_countdown.png|Отсчёт очистки трека}}

<h2 id="patterns">5. Паттерны, вариации, цепочки</h2>
<p>В памяти <b>4 паттерна A–D</b>, у каждого <b>8 вариаций</b>. Паттерн хранит звуки треков, вариации, локи и motion,
MLEN, эффекты. Вариации — разные партии шагов одного паттерна.</p>
<h3 id="cue">Переключение</h3>
<table><thead><tr><th>Комбинация</th><th>Действие</th></tr></thead><tbody>
<tr><td><kbd>GLO</kbd> + белая 1–4</td><td>Поставить в очередь паттерн A–D</td></tr>
<tr><td><kbd>GLO</kbd> + чёрная 1–8</td><td>Поставить в очередь вариацию 1–8 (паттерна в очереди или текущего)</td></tr>
<tr><td><kbd>SAVE</kbd> удерживать 0,4 с (или <kbd>SAVE</kbd> + клавиша)</td><td>Карта переключения: те же клавиши, в верхней строке <code>CUE A1*</code></td></tr>
<tr><td>Та же клавиша ещё раз</td><td>Переключить на следующем шаге</td></tr>
<tr><td>Клавиша играющего паттерна/вариации</td><td>Отменить очередь</td></tr>
</tbody></table>
<p>Когда транспорт стоит, переключение мгновенное. При игре — в конце самого длинного трека или на границе MLEN;
затем все треки начинают с первого шага. <kbd>PLAY</kbd> (стоп) сбрасывает очередь. Пустой паттерн выбрать нельзя (<code>SLOT A EMPTY</code>).</p>
<p><b>Подсветка в карте:</b> белые — паттерн есть (горит), загружен (подмигивает), в очереди (мигает), с несохранёнными правками (дышит);
чёрные — вариация есть (горит), играет (подмигивает), в очереди (мигает).</p>
<h3 id="chain">Цепочки</h3>
<p>Держите <kbd>GLO</kbd> и нажмите <b>две или больше</b> белых 1–4 по порядку. Чёрная клавиша после белой задаёт вариацию этого звена.
Цепочка включается при отпускании <kbd>GLO</kbd>. Пример: <kbd>GLO</kbd> + A, чёрная 1, B, чёрная 3, A, чёрная 2 → <code>A1 B3 A2</code>.</p>
<ul>
<li>До 16 звеньев, повторы разрешены. Каждое звено играет одну длину паттерна (или одну MLEN), затем цепочка идёт по кругу. Пустые паттерны пропускаются.</li>
<li>Цепочка видна на HOME строкой <code>CH A1 B3 A2</code>: текущее звено инверсно, следующее подчёркнуто.</li>
<li>Заканчивается по стопу, LOAD, одиночному переключению паттерна или вариации.</li>
<li>Цепочка не сохраняется.</li>
</ul>

<h2 id="perfmidi">6. Mute, solo, запись, MIDI, Bluetooth</h2>
<h3 id="mute">Mute и solo</h3>
<table><thead><tr><th>Комбинация</th><th>Действие</th></tr></thead><tbody>
<tr><td><kbd>SEQ</kbd> + чёрная 1–6</td><td>Mute трека 1–6: сразу или со следующего такта (меню MUTE MODE)</td></tr>
<tr><td><kbd>ARP</kbd> + чёрная 1–6</td><td>Solo трека: TOGGLE (вкл/выкл) или MOMENTARY (пока клавиша удержана)</td></tr>
<tr><td><kbd>ARP</kbd> + F#5 (чёрная 11)</td><td>Переключить режим solo TOGGLE / MOMENTARY (сохраняется)</td></tr>
<tr><td><kbd>SEL</kbd> + чёрная 1–6</td><td>Выбрать трек</td></tr>
</tbody></table>

<h3 id="rec">Запись</h3>
<ul>
<li><b>TRACKS</b>: тап <kbd>REC</kbd> взводит или снимает запись выбранного трека (<code>T3 REC ARMED</code>).</li>
<li><b>Любая другая страница</b>: тап <kbd>REC</kbd> — <b>REC ALL</b>, взведены все треки. Повторный тап возвращает прежнее состояние.</li>
<li>REC не запускает транспорт. Запись начинается, когда идёт <kbd>PLAY</kbd>.</li>
<li>Играйте треки белыми 1–6 на HOME или страницах звука, нотами в режиме KEYS или с внешнего MIDI: удары ложатся на шаги взведённых треков,
квантование — меню REC QUANT (OFF сохраняет вашу точность через NUDGE). На PATTERN и в режиме STEPS белые клавиши — шаги, там пальцами не записать.</li>
<li><b>Motion:</b> повороты ручек записываются, если выбранный трек взведён и идёт воспроизведение.</li>
</ul>

<h3 id="keys">Режим KEYS</h3>
<p>KEYS включён при старте (горит SEL); тап <kbd>SEL</kbd> выключает и включает его. Пока горит SEL, все 27 клавиш играют выбранный трек хроматически на любой странице, секвенсер клавишами не управляется.
<kbd>OCT-</kbd>/<kbd>OCT+</kbd> — октава −2…+2. Тап <kbd>ENV</kbd> выключает KEYS и включает STEPS, и наоборот.</p>
<p><b>MODE трека в KEYS.</b> MODE выбранного трека (страница ARP) действует и на клавиши. RTRG повторяет удержанную ноту с частотой RATE (с GATE и FADE), пока клавиша нажата.
ARP1 арпеджирует ноту удержанной клавиши; новая клавиша (моно, легато как обычно) начинает арпеджио с новой ноты. ARP2 арпеджирует все удержанные клавиши (до 8),
отсортированные по высоте, в порядке DIR на RANGE октав; добавление и отпускание клавиш меняет набор нот, не сбивая ритм, а отпускание последней останавливает арпеджио
(гейт последней ноты доигрывает). Темп и свинг — как у трека: при игре по шагам трека, при остановке — свободно в темпе от момента нажатия. Отпускание, STOP,
смена трека или выход из KEYS останавливают ролл. Повторы не идут в MIDI OUT; при записи пишется нажатая нота, а не повторы (их потом играет MODE шага).</p>

<h3 id="midiout">MIDI OUT</h3>
<p>Страница GLO → MIDI OUT. <code>OUT</code> — куда слать (USB; в сборке с Bluetooth ещё BLE/BOTH). <code>T1CH…T6CH</code> — канал выбранного трека
(OFF — трек не отправляет). Отправляется то, что играет секвенсор: шаги, рэтчеты, NUDGE, роллы ARP/RTRG.
Машины с высотой тона шлют сыгранную ноту, ударные — NOTE трека. Velocity 127 (акцент) или 96. Клок и старт/стоп — меню MIDI SYNC.</p>

<h3 id="midikbd">MIDI-клавиатура</h3>
<p>На MIDI OUT поставьте <code>KBD</code> в TRK (канал выбранного трека) или 1–16. Режим KEYS включится сам, и 27 клавиш начнут слать ноты
(velocity 100) в USB и, если есть связь, в Bluetooth. <kbd>OCT-</kbd>/<kbd>OCT+</kbd> — октава −4…+4. <code>LOCL OFF</code> глушит собственный звук FM-1.
HOME показывает <code>MIDI KBD CH1 OCT+1</code> (<code>CH-</code>, если канал трека OFF).</p>

<h3 id="midiin">MIDI-вход, клок, MIDI LEARN</h3>
<p>Меню: CLOCK (INT / USB / TRS …), MIDI CHANNEL, MIDI ROUTE, MIDI THRU. <b>MIDI LEARN:</b> SAVE → COPY, <kbd class="k">KNOB 4</kbd> LEARN ON.
Затем выберите цель (повернуть ручку, нажать PLAY, REC, ALGORITHM или клавишу PERFORM) и пошлите CC с контроллера.
<kbd>OCT-</kbd> стирает привязку цели (без цели — вопрос <code>CLEAR ALL LEARNED?</code>), <kbd>OCT+</kbd> — готово.</p>

<h3 id="bt">Bluetooth MIDI</h3>
{{screen:bt/00_menu.png|MENU: BLUETOOTH}}
<p>Только в сборке с Bluetooth. Меню (удержание <kbd>HOME</kbd>) → пункт <code>BLUETOOTH</code> → <kbd>OCT+</kbd> OPEN.
На этой странице включается Bluetooth, роль, автостарт и сохранённые устройства; <kbd>OCT-</kbd> — назад, <kbd>OCT+</kbd> — OK.
Для компьютера или планшета FM-1 виден как <b>Felucca FM-1</b>. Сообщения <code>BT CONNECTED</code> / <code>BT LOST</code>.
AUTOSTART включает Bluetooth через 5 с после старта. Если держать <kbd>OCT-</kbd> + <kbd>OCT+</kbd> в этот момент,
автостарт пропускается; в безопасном режиме Bluetooth не включается.</p>
{{/screen}}

<h2 id="cheat">7. Сводная таблица комбинаций</h2>
<table class="cheat"><thead><tr><th>Тема</th><th>Комбинация</th><th>Действие</th></tr></thead><tbody>
<tr><td rowspan="9">Навигация</td><td><kbd>HOME</kbd></td><td>HOME ⇄ TRACKS</td></tr>
<tr><td><kbd>HOME</kbd> 0,7 с</td><td>Меню устройства</td></tr>
<tr><td><kbd>EDIT</kbd> / удержание 0,4 с</td><td>SOUND → ARP / список пресетов</td></tr>
<tr><td><kbd>ARP</kbd> + <kbd>EDIT</kbd></td><td>Страницы ARP 1 / 2</td></tr>
<tr><td><kbd>FX</kbd> / удержание 0,4 с</td><td>Страницы FX / PERFORM</td></tr>
<tr><td><kbd>LFO</kbd>, <kbd>SEQ</kbd>, <kbd>GLO</kbd> (тап), <kbd>SAVE</kbd> (тап)</td><td>LFO, PATTERN, глобальные страницы, PROJECT/TOOLS/COPY</td></tr>
<tr><td><kbd class="k">SELECT</kbd></td><td>Страница вперёд/назад; на TRACKS — BPM</td></tr>
<tr><td><kbd class="k">ALGORITHM</kbd></td><td>Трек</td></tr>
<tr><td><kbd class="k">PRESETS</kbd></td><td>Следующий/предыдущий пресет трека</td></tr>
<tr><td rowspan="4">Треки</td><td><kbd>SEL</kbd> + чёрная 1–6, белая 1–6 на HOME/звуке</td><td>Выбор трека</td></tr>
<tr><td><kbd>SEQ</kbd> + чёрная 1–6</td><td>Mute</td></tr>
<tr><td><kbd>ARP</kbd> + чёрная 1–6</td><td>Solo</td></tr>
<tr><td><kbd>ARP</kbd> + F#5</td><td>Режим solo TOGGLE/MOMENTARY</td></tr>
<tr><td rowspan="7">Шаги</td><td>тап шага / удержание 0,3 с</td><td>Вкл/выкл / удержанный шаг</td></tr>
<tr><td>шаг + <kbd class="k">KNOB 1–4</kbd> (PATTERN, TRACKS)</td><td>PROB / RATCH / ACC / NUDGE</td></tr>
<tr><td><kbd>OCT+</kbd> + шаг</td><td>Акцент</td></tr>
<tr><td><kbd>OCT-</kbd> / <kbd>OCT+</kbd> (PATTERN, TRACKS)</td><td>Банк шагов</td></tr>
<tr><td><kbd>ENV</kbd> + <kbd>OCT-</kbd>/<kbd>OCT+</kbd></td><td>Банк шагов на любой странице</td></tr>
<tr><td><kbd>REC</kbd> 0,4 с</td><td>FILL, пока удержана</td></tr>
<tr><td><kbd>ENV</kbd></td><td>Режим STEPS вкл/выкл</td></tr>
<tr><td rowspan="4">Локи</td><td>STEPS: шаг + ручка</td><td>Parameter lock</td></tr>
<tr><td>STEPS: шаг + MODEL</td><td>Machine lock</td></tr>
<tr><td>шаг + <kbd>OCT-</kbd></td><td>Стереть локи страницы</td></tr>
<tr><td>шаг + <kbd>OCT-</kbd> 0,6 с</td><td>Стереть все локи шага</td></tr>
<tr><td rowspan="5">Копирование</td><td>шаг(и) + <kbd>REC</kbd> / <kbd>PLAY</kbd></td><td>Копировать / вставить шаги</td></tr>
<tr><td><kbd>SAVE</kbd> + <kbd>REC</kbd> / <kbd>PLAY</kbd></td><td>Копировать / вставить трек</td></tr>
<tr><td><kbd>GLO</kbd> + <kbd>REC</kbd> / <kbd>PLAY</kbd></td><td>Копировать / вставить паттерн</td></tr>
<tr><td><kbd>REC</kbd> + <kbd>SEL</kbd> 5 с</td><td>Очистить трек</td></tr>
<tr><td>Всплывающее окно + <kbd>OCT-</kbd></td><td>Отменить правку ручки</td></tr>
<tr><td rowspan="4">Паттерны</td><td><kbd>GLO</kbd> + белая 1–4</td><td>Паттерн A–D в очередь</td></tr>
<tr><td><kbd>GLO</kbd> + чёрная 1–8</td><td>Вариация 1–8 в очередь</td></tr>
<tr><td><kbd>GLO</kbd> + 2+ белых (+ чёрные)</td><td>Цепочка</td></tr>
<tr><td><kbd>SAVE</kbd> 0,4 с + клавиши</td><td>Карта переключения</td></tr>
<tr><td rowspan="4">Транспорт и запись</td><td><kbd>PLAY</kbd></td><td>Старт/стоп</td></tr>
<tr><td><kbd>REC</kbd> на TRACKS</td><td>Взвести выбранный трек</td></tr>
<tr><td><kbd>REC</kbd> на других страницах</td><td>REC ALL вкл/выкл</td></tr>
<tr><td><kbd>SEL</kbd> (тап)</td><td>KEYS; <kbd>OCT-</kbd>/<kbd>OCT+</kbd> — октава</td></tr>
<tr><td rowspan="2">Диалоги и меню</td><td><kbd>OCT+</kbd> / <kbd>OCT-</kbd></td><td>YES / NO (в меню: следующее значение / выход)</td></tr>
<tr><td>LOAD/SAVE/PASTE/TOOLS</td><td>Два щелчка ручки</td></tr>
<tr><td rowspan="4">Включение</td><td>держать <kbd>SEQ</kbd></td><td>SAFE START</td></tr>
<tr><td>держать <kbd>OCT-</kbd> + <kbd>OCT+</kbd></td><td>Калибровка кнопок и ручек</td></tr>
<tr><td><kbd>OCT-</kbd> + <kbd>OCT+</kbd> 5 с во время работы</td><td>Режим загрузчика чипа (UBOOT), см. раздел 8</td></tr>
<tr><td>любая клавиша на заставке</td><td>Пропустить заставку</td></tr>
</tbody></table>

<h2 id="safety">8. Безопасность и восстановление</h2>
<h3 id="safestart">SAFE START</h3>
{{screen:safe_start.png|SAFE START}}
<p>Выключите FM-1, <b>держите <kbd>SEQ</kbd></b> и включите. Появится <code>SAFE START</code> /
<code>NO AUDIO - USB UPDATE READY</code> / <code>POWER OFF TO LEAVE</code>.</p>
<ul>
<li>Не загружается ни один паттерн, нет звука, все светодиоды погашены, панель не реагирует, Bluetooth не стартует.</li>
<li>USB работает: можно обновить прошивку или вернуть заводскую.</li>
<li>Чтобы выйти, выключите питание.</li>
</ul>
<p>Используйте после экрана сбоя, повторяющихся перезагрузок или пустого экрана.</p>
{{/screen}}
<h3 id="stock">Возврат на заводскую прошивку M-VAVE</h3>
<ol>
<li>Скачайте с <span class="mono">www.m-vave.com/download</span> программу <b>M-UPGRADE</b> (Downloads → PC Software) и официальную прошивку <b>FM-1 V15</b> (PC Firmware).</li>
<li>Подключите FM-1 кабелем USB с данными (при проблемах — в SAFE START) и прошейте V15 через M-UPGRADE.
Можно также выбрать <i>Return to official V15</i> в веб-установщике Hortator.</li>
</ol>
<h3 id="never">Что никогда не меняется</h3>
<ul>
<li><b>Загрузчик обновлений</b> (на USB виден как <code>Felucca Update</code>, протокол M-UPGRADE) пишет только область прошивки
(флеш с 0x4000). Загрузочную область чипа он не трогает, поэтому путь обновления остаётся всегда.</li>
<li>Встроенный в чип режим загрузки (UBOOT) стереть невозможно. Вход: держать <kbd>OCT-</kbd> + <kbd>OCT+</kbd> во время работы.
Через 2 с начнётся отсчёт (отпустите — отмена), на 5 с — <code>UBOOT</code>. Нужен только для восстановления с компьютера
(<span class="mono">docs/RECOVERY.md</span> в репозитории: инструмент пишет только область 0x4000–0x93000 и не трогает загрузочную область,
ключ чипа и область данных).</li>
<li>Паттерны, сохранённые 8-трековым Hortator, FREITAKT не читает (другой формат).</li>
</ul>
<div class="warn">Прошивка неофициальная, вы используете её на свой риск. M-VAVE за неё не отвечает.</div>
'''
